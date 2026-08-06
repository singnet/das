#define LOG_LEVEL INFO_LEVEL
#include "RemoteAtomDBPeer.h"

#include <algorithm>
#include <deque>
#include <fstream>
#include <sstream>
#include <thread>
#include <unordered_map>

#include "Atom.h"
#include "InMemoryDBAPITypes.h"
#include "Link.h"
#include "Logger.h"
#include "Node.h"
#include "Utils.h"
#include "expression_hasher.h"

using namespace atomdb;
using namespace atomdb_api_types;
using namespace atoms;
using namespace commons;

RemoteAtomDBPeer::RemoteAtomDBPeer(shared_ptr<AtomDB> remote_atomdb,
                                   shared_ptr<AtomDB> local_persistence,
                                   const string& uid)
    : uid_(uid),
      write_buffer_(make_shared<InMemoryDB>(uid + "_wb")),
      read_cache_(make_shared<InMemoryDB>(uid + "_rc")),
      atomdb_(remote_atomdb),
      local_persistence_(local_persistence) {
    start_cleanup_thread();
}

RemoteAtomDBPeer::~RemoteAtomDBPeer() { stop_cleanup_thread(); }

bool RemoteAtomDBPeer::allow_nested_indexing() { return atomdb_->allow_nested_indexing(); }

bool RemoteAtomDBPeer::composite_type_enabled() const {
    return local_persistence_ && local_persistence_->composite_type_enabled();
}

shared_ptr<InMemoryDB> RemoteAtomDBPeer::write_buffer() const {
    lock_guard<mutex> lock(peer_mutex_);
    return write_buffer_;
}

shared_ptr<InMemoryDB> RemoteAtomDBPeer::read_cache() const {
    lock_guard<mutex> lock(peer_mutex_);
    return read_cache_;
}

void RemoteAtomDBPeer::invalidate_fetched_templates() {
    lock_guard<mutex> lock(peer_mutex_);
    fetched_link_templates_.clear();
}

shared_ptr<Atom> RemoteAtomDBPeer::get_atom(const string& handle) {
    // Snapshot the two in-memory layers without holding the mutex across I/O.
    auto wb = write_buffer();
    auto rc = read_cache();

    // Dirty writes always win over durable / warmed copies.
    if (auto atom = wb->get_atom(handle)) {
        return atom;
    }

    if (local_persistence_) {
        auto atom = local_persistence_->get_atom(handle);
        if (atom) {
            LOG_DEBUG("[RemoteDB(" << uid_ << ")] get_atom(" << handle
                                   << ") <- local_persistence (warmed into read_cache)");
            // Warm only once: local wins over read_cache on this path anyway, so re-cloning
            // the atom into the trie on every hit would be pure churn. The warmed copy still
            // serves query_for_targets and the facade cache probes.
            if (!rc->atom_exists(handle)) {
                rc->add_atom(atom.get());
            }
            return atom;
        }
    }

    if (auto atom = rc->get_atom(handle)) {
        return atom;
    }

    auto atom = atomdb_->get_atom(handle);
    if (atom) {
        LOG_DEBUG("[RemoteDB(" << uid_ << ")] get_atom(" << handle << ") <- remote atomdb (warmed)");
        rc->add_atom(atom.get());
        return atom;
    }

    LOG_DEBUG("[RemoteDB(" << uid_ << ")] get_atom(" << handle << ") miss");
    return nullptr;
}

shared_ptr<Node> RemoteAtomDBPeer::get_node(const string& handle) {
    auto atom = get_atom(handle);
    return dynamic_pointer_cast<Node>(atom);
}

shared_ptr<Link> RemoteAtomDBPeer::get_link(const string& handle) {
    auto atom = get_atom(handle);
    return dynamic_pointer_cast<Link>(atom);
}

shared_ptr<Atom> RemoteAtomDBPeer::get_cached_atom(const string& handle) {
    if (auto atom = write_buffer()->get_atom(handle)) {
        return atom;
    }
    return read_cache()->get_atom(handle);
}

shared_ptr<Node> RemoteAtomDBPeer::get_cached_node(const string& handle) {
    auto atom = get_cached_atom(handle);
    return dynamic_pointer_cast<Node>(atom);
}

shared_ptr<Link> RemoteAtomDBPeer::get_cached_link(const string& handle) {
    auto atom = get_cached_atom(handle);
    return dynamic_pointer_cast<Link>(atom);
}

vector<shared_ptr<Atom>> RemoteAtomDBPeer::get_matching_atoms(bool is_toplevel, Atom& key) {
    return get_matching_atoms(is_toplevel, key, false);
}

vector<shared_ptr<Atom>> RemoteAtomDBPeer::get_matching_atoms(bool is_toplevel,
                                                              Atom& key,
                                                              bool local_only) {
    vector<shared_ptr<Atom>> result;
    set<string> seen_handles;

    auto merge_results = [&](const vector<shared_ptr<Atom>>& atoms) {
        for (const auto& atom : atoms) {
            string h = atom->handle();
            if (seen_handles.insert(h).second) {
                result.push_back(atom);
            }
        }
    };

    merge_results(write_buffer()->get_matching_atoms(is_toplevel, key));
    merge_results(read_cache()->get_matching_atoms(is_toplevel, key));
    if (local_persistence_) {
        merge_results(local_persistence_->get_matching_atoms(is_toplevel, key));
    }

    if (!local_only && atomdb_) {
        merge_results(atomdb_->get_matching_atoms(is_toplevel, key));
    }

    return result;
}

void RemoteAtomDBPeer::feed_cache_from_handle_set(shared_ptr<HandleSet> handle_set) {
    if (!handle_set) return;

    auto it = handle_set->get_iterator();
    if (!it) return;

    LOG_DEBUG("[RemoteDB(" << uid_ << ")] feed_cache_from_handle_set: warming cache with "
                           << handle_set->size() << " handles");
    while (true) {
        char* handle_cstr = it->next();
        if (!handle_cstr) break;

        string handle(handle_cstr);
        get_atom(handle);
    }
}

void RemoteAtomDBPeer::merge_handle_set(shared_ptr<HandleSet> source,
                                        shared_ptr<HandleSetInMemory> dest,
                                        set<string>& seen,
                                        bool copy_metadata) {
    if (!source) return;
    auto it = source->get_iterator();
    char* h;
    while ((h = it->next()) != nullptr) {
        string s(h);
        if (seen.insert(s).second) {
            // copy_metadata must only be set when the source backend supports nested indexing;
            // otherwise get_*_by_handle may be unsupported (e.g. HandleSetRedis raises).
            if (copy_metadata) {
                dest->add_handle(
                    s, source->get_metta_expressions_by_handle(s), source->get_assignments_by_handle(s));
            } else {
                dest->add_handle(s);
            }
        }
    }
}

shared_ptr<HandleSet> RemoteAtomDBPeer::query_for_pattern(const LinkSchema& link_schema) {
    auto result = make_shared<HandleSetInMemory>();
    set<string> seen;

    auto merge_memory = [&](const shared_ptr<InMemoryDB>& db) {
        merge_handle_set(db->query_for_pattern(link_schema), result, seen, db->allow_nested_indexing());
    };

    auto merge_local_persistence = [&]() {
        if (!local_persistence_) return;
        merge_handle_set(local_persistence_->query_for_pattern(link_schema),
                         result,
                         seen,
                         local_persistence_->allow_nested_indexing());
    };

    bool cache_hit;
    {
        lock_guard<mutex> lock(peer_mutex_);
        cache_hit = fetched_link_templates_.count(link_schema.handle()) > 0;
    }

    if (cache_hit) {
        LOG_DEBUG("[RemoteDB(" << uid_ << ")] query_for_pattern(" << link_schema.handle()
                               << ") cache-hit");
        merge_memory(write_buffer());
        merge_memory(read_cache());
        merge_local_persistence();
        LOG_DEBUG("[RemoteDB(" << uid_ << ")] query_for_pattern(" << link_schema.handle() << ") -> "
                               << result->size() << " handles");
        return result;
    }

    LOG_DEBUG("[RemoteDB(" << uid_ << ")] query_for_pattern(" << link_schema.handle()
                           << ") cache-miss, fetching from remote atomdb");

    shared_ptr<HandleSet> remote_handle_set = atomdb_->query_for_pattern(link_schema);
    if (remote_handle_set) {
        feed_cache_from_handle_set(remote_handle_set);
        // Merge remote first when it carries nested metadata. InMemoryDB has no
        // metta/assignments — if it fills `seen` first, the remote metadata is skipped.
        merge_handle_set(remote_handle_set, result, seen, atomdb_->allow_nested_indexing());
    }

    merge_memory(write_buffer());
    merge_memory(read_cache());
    merge_local_persistence();

    {
        lock_guard<mutex> lock(peer_mutex_);
        fetched_link_templates_.insert(link_schema.handle());
    }

    LOG_DEBUG("[RemoteDB(" << uid_ << ")] query_for_pattern(" << link_schema.handle() << ") -> "
                           << result->size() << " handles");
    return result;
}

shared_ptr<HandleList> RemoteAtomDBPeer::query_for_targets(const string& handle) {
    if (auto result = write_buffer()->query_for_targets(handle)) {
        return result;
    }
    if (auto result = read_cache()->query_for_targets(handle)) {
        return result;
    }

    if (local_persistence_) {
        auto result = local_persistence_->query_for_targets(handle);
        if (result) {
            LOG_DEBUG("[RemoteDB(" << uid_ << ")] query_for_targets(" << handle
                                   << ") <- local_persistence");
            return result;
        }
    }

    LOG_DEBUG("[RemoteDB(" << uid_ << ")] query_for_targets(" << handle << ") <- remote atomdb");
    return atomdb_->query_for_targets(handle);
}

shared_ptr<HandleSet> RemoteAtomDBPeer::query_for_incoming_set(const string& handle) {
    auto result = make_shared<HandleSetInMemory>();
    set<string> seen;

    merge_handle_set(write_buffer()->query_for_incoming_set(handle), result, seen);
    merge_handle_set(read_cache()->query_for_incoming_set(handle), result, seen);
    if (local_persistence_) {
        merge_handle_set(local_persistence_->query_for_incoming_set(handle), result, seen);
    }

    merge_handle_set(atomdb_->query_for_incoming_set(handle), result, seen);

    LOG_DEBUG("[RemoteDB(" << uid_ << ")] query_for_incoming_set(" << handle << ") -> " << result->size()
                           << " handles");
    return result;
}

bool RemoteAtomDBPeer::atom_exists(const string& handle) {
    if (write_buffer()->atom_exists(handle)) return true;
    if (read_cache()->atom_exists(handle)) return true;
    if (local_persistence_ && local_persistence_->atom_exists(handle)) return true;
    if (atomdb_ && atomdb_->atom_exists(handle)) return true;
    return false;
}

bool RemoteAtomDBPeer::node_exists(const string& handle) {
    if (write_buffer()->node_exists(handle)) return true;
    if (read_cache()->node_exists(handle)) return true;
    if (local_persistence_ && local_persistence_->node_exists(handle)) return true;
    if (atomdb_ && atomdb_->node_exists(handle)) return true;
    return false;
}

bool RemoteAtomDBPeer::link_exists(const string& handle) {
    if (write_buffer()->link_exists(handle)) return true;
    if (read_cache()->link_exists(handle)) return true;
    if (local_persistence_ && local_persistence_->link_exists(handle)) return true;
    if (atomdb_ && atomdb_->link_exists(handle)) return true;
    return false;
}

set<string> RemoteAtomDBPeer::atoms_exist(const vector<string>& handles) {
    set<string> result;
    set<string> remaining(handles.begin(), handles.end());

    auto from_source = [&](AtomDB& db) {
        vector<string> to_check(remaining.begin(), remaining.end());
        if (to_check.empty()) return;
        for (const auto& h : db.atoms_exist(to_check)) {
            result.insert(h);
            remaining.erase(h);
        }
    };

    from_source(*write_buffer());
    if (!remaining.empty()) from_source(*read_cache());
    if (local_persistence_ && !remaining.empty()) from_source(*local_persistence_);
    if (atomdb_ && !remaining.empty()) from_source(*atomdb_);

    return result;
}

set<string> RemoteAtomDBPeer::nodes_exist(const vector<string>& handles) {
    set<string> result;
    set<string> remaining(handles.begin(), handles.end());

    auto from_source = [&](AtomDB& db) {
        vector<string> to_check(remaining.begin(), remaining.end());
        if (to_check.empty()) return;
        for (const auto& h : db.nodes_exist(to_check)) {
            result.insert(h);
            remaining.erase(h);
        }
    };

    from_source(*write_buffer());
    if (!remaining.empty()) from_source(*read_cache());
    if (local_persistence_ && !remaining.empty()) from_source(*local_persistence_);
    if (atomdb_ && !remaining.empty()) from_source(*atomdb_);

    return result;
}

set<string> RemoteAtomDBPeer::links_exist(const vector<string>& handles) {
    set<string> result;
    set<string> remaining(handles.begin(), handles.end());

    auto from_source = [&](AtomDB& db) {
        vector<string> to_check(remaining.begin(), remaining.end());
        if (to_check.empty()) return;
        for (const auto& h : db.links_exist(to_check)) {
            result.insert(h);
            remaining.erase(h);
        }
    };

    from_source(*write_buffer());
    if (!remaining.empty()) from_source(*read_cache());
    if (local_persistence_ && !remaining.empty()) from_source(*local_persistence_);
    if (atomdb_ && !remaining.empty()) from_source(*atomdb_);

    return result;
}

string RemoteAtomDBPeer::add_atom(const atoms::Atom* atom, const atoms::Merger* merger) {
    vector<Atom*> atoms = {const_cast<atoms::Atom*>(atom)};
    auto handles = add_atoms(atoms, false, merger);
    return handles.empty() ? "" : handles[0];
}

string RemoteAtomDBPeer::add_node(const atoms::Node* node, const atoms::Merger* merger) {
    vector<Node*> nodes = {const_cast<atoms::Node*>(node)};
    auto handles = add_nodes(nodes, false, merger);
    return handles.empty() ? "" : handles[0];
}

string RemoteAtomDBPeer::add_link(const atoms::Link* link, const atoms::Merger* merger) {
    vector<Link*> links = {const_cast<atoms::Link*>(link)};
    auto handles = add_links(links, false, merger);
    return handles.empty() ? "" : handles[0];
}

// Writes snapshot the write_buffer_ pointer and operate outside peer_mutex_. A racing
// release_cache() may swap the buffer out from under us: either this add lands in the
// old buffer (which is about to be persisted) or the new one (persisted on the next
// release). Writes are never lost; the mutex is no longer held across the batch.

vector<string> RemoteAtomDBPeer::add_atoms(const vector<atoms::Atom*>& atoms,
                                           bool is_transactional,
                                           const atoms::Merger* merger) {
    if (is_readonly()) return vector<string>();

    auto wb = write_buffer();
    invalidate_fetched_templates();
    return wb->add_atoms(atoms, is_transactional, merger);
}

vector<string> RemoteAtomDBPeer::add_nodes(const vector<atoms::Node*>& nodes,
                                           bool is_transactional,
                                           const atoms::Merger* merger) {
    if (is_readonly()) return vector<string>();

    auto wb = write_buffer();
    invalidate_fetched_templates();
    return wb->add_nodes(nodes, is_transactional, merger);
}

vector<string> RemoteAtomDBPeer::add_links(const vector<atoms::Link*>& links,
                                           bool is_transactional,
                                           const atoms::Merger* merger) {
    if (is_readonly()) return vector<string>();

    auto wb = write_buffer();
    invalidate_fetched_templates();
    return wb->add_links(links, is_transactional, merger);
}

bool RemoteAtomDBPeer::delete_atom(const string& handle, bool delete_link_targets) {
    if (is_readonly()) return false;

    bool wb_ok = write_buffer()->delete_atom(handle, delete_link_targets);
    bool rc_ok = read_cache()->delete_atom(handle, delete_link_targets);
    bool local_ok = local_persistence_->delete_atom(handle, delete_link_targets);
    if (wb_ok || rc_ok || local_ok) {
        invalidate_fetched_templates();
    }
    return wb_ok || rc_ok || local_ok;
}

bool RemoteAtomDBPeer::delete_node(const string& handle, bool delete_link_targets) {
    if (is_readonly()) return false;

    bool wb_ok = write_buffer()->delete_node(handle, delete_link_targets);
    bool rc_ok = read_cache()->delete_node(handle, delete_link_targets);
    bool local_ok = local_persistence_->delete_node(handle, delete_link_targets);
    if (wb_ok || rc_ok || local_ok) {
        invalidate_fetched_templates();
    }
    return wb_ok || rc_ok || local_ok;
}

bool RemoteAtomDBPeer::delete_link(const string& handle, bool delete_link_targets) {
    if (is_readonly()) return false;

    bool wb_ok = write_buffer()->delete_link(handle, delete_link_targets);
    bool rc_ok = read_cache()->delete_link(handle, delete_link_targets);
    bool local_ok = local_persistence_->delete_link(handle, delete_link_targets);
    if (wb_ok || rc_ok || local_ok) {
        invalidate_fetched_templates();
    }
    return wb_ok || rc_ok || local_ok;
}

// Batch deletes count distinct handles removed from at least one layer. The layers overlap
// (a handle can live in write buffer, read cache and local persistence at once), so summing
// per-layer counts would over-report.

uint RemoteAtomDBPeer::delete_atoms(const vector<string>& handles, bool delete_link_targets) {
    if (is_readonly()) return 0;

    uint deleted = 0;
    for (const auto& handle : handles) {
        if (delete_atom(handle, delete_link_targets)) deleted++;
    }
    return deleted;
}

uint RemoteAtomDBPeer::delete_nodes(const vector<string>& handles, bool delete_link_targets) {
    if (is_readonly()) return 0;

    uint deleted = 0;
    for (const auto& handle : handles) {
        if (delete_node(handle, delete_link_targets)) deleted++;
    }
    return deleted;
}

uint RemoteAtomDBPeer::delete_links(const vector<string>& handles, bool delete_link_targets) {
    if (is_readonly()) return 0;

    uint deleted = 0;
    for (const auto& handle : handles) {
        if (delete_link(handle, delete_link_targets)) deleted++;
    }
    return deleted;
}

void RemoteAtomDBPeer::re_index_patterns(bool flush_patterns) {
    if (is_readonly()) return;

    write_buffer()->re_index_patterns(flush_patterns);
    read_cache()->re_index_patterns(flush_patterns);
    if (local_persistence_) {
        local_persistence_->re_index_patterns(flush_patterns);
    }
}

// Counts define the peer's atom population as write_buffer + local_persistence (dirty +
// durable). The read cache is excluded: it is a non-authoritative view of atoms that already
// live in local persistence or on the remote backend, so including it would double-count.
// Note: a staged update of an atom that already exists locally is still counted twice; the
// result is an upper bound, not an exact distinct count.

size_t RemoteAtomDBPeer::node_count() const {
    size_t count = write_buffer()->node_count();
    if (local_persistence_) {
        count += local_persistence_->node_count();
    }
    return count;
}

size_t RemoteAtomDBPeer::link_count() const {
    size_t count = write_buffer()->link_count();
    if (local_persistence_) {
        count += local_persistence_->link_count();
    }
    return count;
}

size_t RemoteAtomDBPeer::atom_count() const {
    size_t count = write_buffer()->atom_count();
    if (local_persistence_) {
        count += local_persistence_->atom_count();
    }
    return count;
}

void RemoteAtomDBPeer::fetch(const LinkSchema& link_schema) {
    {
        lock_guard<mutex> lock(peer_mutex_);
        if (fetched_link_templates_.count(link_schema.handle()) > 0) {
            return;
        }
    }

    LOG_DEBUG("[RemoteDB(" << uid_ << ")] fetch(" << link_schema.handle()
                           << ") prefetching from remote atomdb");
    auto result = atomdb_->query_for_pattern(link_schema);
    if (!result) {
        return;
    }

    feed_cache_from_handle_set(result);

    lock_guard<mutex> lock(peer_mutex_);
    fetched_link_templates_.insert(link_schema.handle());
}

void RemoteAtomDBPeer::persist_atoms_to_local(const vector<shared_ptr<Atom>>& atoms) {
    if (!local_persistence_ || atoms.empty()) {
        return;
    }

    // Build dependency closure from this peer's own backends only (write_buffer already
    // collected into `atoms`, plus atomdb_ / local_persistence). Cross-peer federation is
    // intentionally not attempted here; local_persistence is expected to run with
    // composite_type_enabled=false so links can be written without resolving foreign targets
    // for composite_type_hash.
    unordered_map<string, shared_ptr<Atom>> atoms_by_handle;
    atoms_by_handle.reserve(atoms.size());
    for (const auto& atom : atoms) {
        if (atom) {
            atoms_by_handle[atom->handle()] = atom;
        }
    }

    deque<string> missing_targets_queue;
    auto enqueue_missing_targets = [&](const shared_ptr<Atom>& atom) {
        if (!atom || atom->arity() == 0) return;
        auto* link = dynamic_cast<Link*>(atom.get());
        if (link == nullptr) return;
        for (const auto& target : link->targets) {
            if (atoms_by_handle.find(target) == atoms_by_handle.end() &&
                !local_persistence_->atom_exists(target)) {
                missing_targets_queue.push_back(target);
            }
        }
    };

    for (const auto& [_, atom] : atoms_by_handle) {
        enqueue_missing_targets(atom);
    }

    while (!missing_targets_queue.empty()) {
        string target_handle = missing_targets_queue.front();
        missing_targets_queue.pop_front();

        if (atoms_by_handle.find(target_handle) != atoms_by_handle.end() ||
            local_persistence_->atom_exists(target_handle)) {
            continue;
        }

        auto fetched_atom = atomdb_->get_atom(target_handle);
        if (!fetched_atom) {
            LOG_DEBUG("[RemoteDB(" << uid_ << ")] persist_atoms_to_local: target " << target_handle
                                   << " not available on this peer (composite_type disabled peers "
                                      "may still persist the link)");
            continue;
        }
        atoms_by_handle[target_handle] = fetched_atom;
        enqueue_missing_targets(fetched_atom);
    }

    vector<Node*> nodes;
    vector<Link*> links;
    nodes.reserve(atoms_by_handle.size());
    links.reserve(atoms_by_handle.size());
    for (const auto& [_, atom] : atoms_by_handle) {
        if (!atom) continue;
        if (atom->arity() == 0) {
            auto* node = dynamic_cast<Node*>(atom.get());
            if (node != nullptr) nodes.push_back(node);
        } else {
            auto* link = dynamic_cast<Link*>(atom.get());
            if (link != nullptr) links.push_back(link);
        }
    }

    // Overwrite in place via the backend's upsert semantics (RedisMongoDB replace_one upsert=true).
    // Do NOT delete first: RedisMongoDB::delete_atom cascades over the atom's incoming set, which
    // would wipe large parts of the base KB when a dirty handle is a shared node.
    if (!nodes.empty()) {
        local_persistence_->add_nodes(nodes);
    }

    if (links.empty()) {
        return;
    }

    // When composite_type is disabled, backends skip target-existence checks — persist links
    // directly even if some targets live only on other peers.
    if (!local_persistence_->composite_type_enabled()) {
        local_persistence_->add_links(links);
        return;
    }

    // Persist links in dependency-safe batches when composite_type requires local targets.
    vector<Link*> remaining_links = links;
    while (!remaining_links.empty()) {
        vector<Link*> batch;
        vector<Link*> pending;
        for (auto* link : remaining_links) {
            bool all_targets_exist = true;
            for (const auto& target : link->targets) {
                if (!local_persistence_->atom_exists(target)) {
                    all_targets_exist = false;
                    break;
                }
            }
            if (all_targets_exist) {
                batch.push_back(link);
            } else {
                pending.push_back(link);
            }
        }
        if (batch.empty()) {
            LOG_ERROR("[RemoteDB(" << uid_ << ")] persist_atoms_to_local: dropping " << pending.size()
                                   << " links with unresolved targets (writes will be lost after "
                                      "cache release)");
            break;
        }
        local_persistence_->add_links(batch);
        remaining_links = pending;
    }
}

void RemoteAtomDBPeer::release_cache(bool /*persist_to_local*/, bool /*persist_entire_cache*/) {
    shared_ptr<InMemoryDB> old_write_buffer;
    shared_ptr<InMemoryDB> old_read_cache;

    {
        lock_guard<mutex> lock(peer_mutex_);
        bool nothing_to_do = write_buffer_->atom_count() == 0 && read_cache_->atom_count() == 0 &&
                             fetched_link_templates_.empty();
        if (nothing_to_do) {
            return;
        }

        LOG_DEBUG("[RemoteDB(" << uid_ << ")] release_cache: flushing " << write_buffer_->atom_count()
                               << " dirty atoms, dropping " << read_cache_->atom_count()
                               << " read-cached atoms");

        // Swap under the mutex so new writers snapshot a fresh buffer. Dirty atoms on the
        // retired buffer are persisted after in-flight writers (which still hold a shared_ptr
        // to it) finish — see quiescence wait below.
        old_write_buffer = write_buffer_;
        old_read_cache = read_cache_;
        write_buffer_ = make_shared<InMemoryDB>(uid_ + "_wb");
        read_cache_ = make_shared<InMemoryDB>(uid_ + "_rc");
        fetched_link_templates_.clear();
    }

    // Drop read cache immediately; it is never persisted.
    old_read_cache.reset();

    if (!local_persistence_ || !old_write_buffer) {
        return;
    }

    // Wait until no add_* still holds the retired buffer. New writers already snapshot
    // the replacement under peer_mutex_, so once we are the sole owner no further atoms
    // can appear — then flush everything that landed on this generation.
    while (old_write_buffer.use_count() > 1) {
        this_thread::yield();
    }

    auto to_persist = old_write_buffer->get_all_atoms();
    if (to_persist.empty()) {
        return;
    }

    // Durability: a failed flush (backend down, network error) must not lose dirty atoms.
    // Re-stage them into the current write buffer so the next release retries. Re-staging
    // atoms that were already persisted before the failure is safe (upsert semantics).
    try {
        persist_atoms_to_local(to_persist);
    } catch (const exception& e) {
        LOG_ERROR("[RemoteDB(" << uid_ << ")] release_cache: persist failed (" << e.what()
                               << "); re-staging " << to_persist.size()
                               << " atoms for retry on next release");
        restage_atoms(to_persist);
    } catch (...) {
        LOG_ERROR("[RemoteDB(" << uid_ << ")] release_cache: persist failed (unknown exception); "
                               << "re-staging " << to_persist.size()
                               << " atoms for retry on next release");
        restage_atoms(to_persist);
    }
}

void RemoteAtomDBPeer::restage_atoms(const vector<shared_ptr<Atom>>& atoms) {
    vector<Atom*> raw;
    raw.reserve(atoms.size());
    for (const auto& atom : atoms) {
        if (atom) raw.push_back(atom.get());
    }
    if (raw.empty()) return;
    // Snapshot the *current* write buffer; InMemoryDB clones the atoms internally.
    write_buffer()->add_atoms(raw);
}

void RemoteAtomDBPeer::release(const LinkSchema& link_schema, bool persist, bool force) {
    bool schema_cached;
    bool has_dirty_writes;
    bool has_cached_atoms;
    {
        lock_guard<mutex> lock(peer_mutex_);
        schema_cached = fetched_link_templates_.count(link_schema.handle()) > 0;
        has_dirty_writes = write_buffer_->atom_count() > 0;
        has_cached_atoms = read_cache_->atom_count() > 0 || has_dirty_writes;
    }

    if (persist) {
        if (schema_cached || force || has_dirty_writes) {
            LOG_INFO("[RemoteDB(" << uid_ << ")] release(" << link_schema.handle()
                                  << ") flushing write buffer" << (force ? " (forced)" : ""));
            release_cache();
            return;
        }
        if (!is_readonly() && has_cached_atoms) {
            LOG_INFO("[RemoteDB(" << uid_ << ")] release(" << link_schema.handle()
                                  << ") clearing read cache (no dirty writes)");
            release_cache();
            return;
        }
        LOG_DEBUG("[RemoteDB(" << uid_ << ")] release(" << link_schema.handle()
                               << ") no-op (schema not cached, no dirty writes, empty or readonly)");
        return;
    }

    if (schema_cached) {
        lock_guard<mutex> lock(peer_mutex_);
        fetched_link_templates_.erase(link_schema.handle());
    } else {
        LOG_DEBUG("[RemoteDB(" << uid_ << ")] release(" << link_schema.handle()
                               << ") cache already released");
    }
}

double RemoteAtomDBPeer::available_ram() {
    unsigned long total = Utils::get_total_ram();
    if (total == 0) return 0.0;
    return static_cast<double>(Utils::get_current_free_ram()) / static_cast<double>(total);
}

void RemoteAtomDBPeer::auto_cleanup() {
    double ram_fraction = available_ram();
    if (ram_fraction < CRITICAL_RAM_THRESHOLD) {
        LOG_INFO("RemoteAtomDBPeer " << uid_ << ": Low RAM detected (available="
                                     << (ram_fraction * 100.0) << "%), cache release triggered");
        release_cache();
    }
}

bool RemoteAtomDBPeer::thread_one_step() {
    Utils::sleep(1000);
    auto_cleanup();
    return false;
}

void RemoteAtomDBPeer::start_cleanup_thread() {
    if (cleanup_thread_) return;
    cleanup_thread_ =
        make_unique<processor::DedicatedThread>(uid_ + "_remote_atomdb_peer_cleanup", this);
    cleanup_thread_->setup();
    cleanup_thread_->start();
}

void RemoteAtomDBPeer::stop_cleanup_thread() {
    if (cleanup_thread_) {
        cleanup_thread_->stop();
        cleanup_thread_.reset();
    }
}
