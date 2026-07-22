#define LOG_LEVEL INFO_LEVEL
#include "RemoteAtomDBPeer.h"

#include <algorithm>
#include <deque>
#include <fstream>
#include <sstream>
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
      cache_(make_shared<InMemoryDB>(uid)),
      atomdb_(remote_atomdb),
      local_persistence_(local_persistence),
      fetched_link_templates_(make_unique<HandleTrie>(HANDLE_HASH_SIZE - 1)) {
    start_cleanup_thread();
}

RemoteAtomDBPeer::~RemoteAtomDBPeer() { stop_cleanup_thread(); }

bool RemoteAtomDBPeer::allow_nested_indexing() { return atomdb_->allow_nested_indexing(); }

bool RemoteAtomDBPeer::composite_type_enabled() const {
    return local_persistence_ && local_persistence_->composite_type_enabled();
}

shared_ptr<InMemoryDB> RemoteAtomDBPeer::cache() const {
    lock_guard<mutex> lock(peer_mutex_);
    return cache_;
}

shared_ptr<Atom> RemoteAtomDBPeer::get_atom(const string& handle) {
    // local_persistence is authoritative for written/updated atoms (including strength changes
    // that keep the same content-addressed handle). Prefer it over a possibly stale cache.
    if (local_persistence_) {
        auto atom = local_persistence_->get_atom(handle);
        if (atom) {
            LOG_DEBUG("[RemoteDB(" << uid_ << ")] get_atom(" << handle
                                   << ") <- local_persistence (cached)");
            cache()->add_atom(atom.get());
            return atom;
        }
    }

    if (auto atom = cache()->get_atom(handle)) {
        return atom;
    }

    auto atom = atomdb_->get_atom(handle);
    if (atom) {
        LOG_DEBUG("[RemoteDB(" << uid_ << ")] get_atom(" << handle << ") <- remote atomdb (cached)");
        cache()->add_atom(atom.get());
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
    return cache()->get_atom(handle);
}

shared_ptr<Node> RemoteAtomDBPeer::get_cached_node(const string& handle) {
    return cache()->get_node(handle);
}

shared_ptr<Link> RemoteAtomDBPeer::get_cached_link(const string& handle) {
    return cache()->get_link(handle);
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
            if (seen_handles.find(h) == seen_handles.end()) {
                seen_handles.insert(h);
                result.push_back(atom);
            }
        }
    };

    merge_results(cache()->get_matching_atoms(is_toplevel, key));
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

    auto merge_cache = [&]() {
        auto snapshot = cache();
        merge_handle_set(
            snapshot->query_for_pattern(link_schema), result, seen, snapshot->allow_nested_indexing());
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
        cache_hit = fetched_link_templates_->lookup(link_schema.handle()) != nullptr;
    }

    if (cache_hit) {
        LOG_DEBUG("[RemoteDB(" << uid_ << ")] query_for_pattern(" << link_schema.handle()
                               << ") cache-hit");
        merge_cache();
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
        // Merge remote first when it carries nested metadata. Cache (InMemoryDB) has no
        // metta/assignments — if it fills `seen` first, the remote metadata is skipped.
        merge_handle_set(remote_handle_set, result, seen, atomdb_->allow_nested_indexing());
    }

    merge_cache();
    merge_local_persistence();

    {
        lock_guard<mutex> lock(peer_mutex_);
        if (fetched_link_templates_->lookup(link_schema.handle()) == nullptr) {
            // HandleTrie::insert takes ownership and may delete on re-insert — never reuse one
            // EmptyTrieValue* across inserts.
            fetched_link_templates_->insert(link_schema.handle(), new EmptyTrieValue());
        }
    }

    LOG_DEBUG("[RemoteDB(" << uid_ << ")] query_for_pattern(" << link_schema.handle() << ") -> "
                           << result->size() << " handles");
    return result;
}

shared_ptr<HandleList> RemoteAtomDBPeer::query_for_targets(const string& handle) {
    if (auto result = cache()->query_for_targets(handle)) {
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

    merge_handle_set(cache()->query_for_incoming_set(handle), result, seen);
    if (local_persistence_) {
        merge_handle_set(local_persistence_->query_for_incoming_set(handle), result, seen);
    }

    merge_handle_set(atomdb_->query_for_incoming_set(handle), result, seen);

    LOG_DEBUG("[RemoteDB(" << uid_ << ")] query_for_incoming_set(" << handle << ") -> " << result->size()
                           << " handles");
    return result;
}

bool RemoteAtomDBPeer::atom_exists(const string& handle) {
    if (cache()->atom_exists(handle)) return true;
    if (local_persistence_ && local_persistence_->atom_exists(handle)) return true;
    if (atomdb_ && atomdb_->atom_exists(handle)) return true;
    return false;
}

bool RemoteAtomDBPeer::node_exists(const string& handle) {
    if (cache()->node_exists(handle)) return true;
    if (local_persistence_ && local_persistence_->node_exists(handle)) return true;
    if (atomdb_ && atomdb_->node_exists(handle)) return true;
    return false;
}

bool RemoteAtomDBPeer::link_exists(const string& handle) {
    if (cache()->link_exists(handle)) return true;
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

    from_source(*cache());
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

    from_source(*cache());
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

    from_source(*cache());
    if (local_persistence_ && !remaining.empty()) from_source(*local_persistence_);
    if (atomdb_ && !remaining.empty()) from_source(*atomdb_);

    return result;
}

string RemoteAtomDBPeer::add_atom(const atoms::Atom* atom, bool throw_if_exists) {
    vector<Atom*> atoms = {const_cast<atoms::Atom*>(atom)};
    auto handles = add_atoms(atoms, throw_if_exists);
    return handles.empty() ? "" : handles[0];
}

string RemoteAtomDBPeer::add_node(const atoms::Node* node, bool throw_if_exists) {
    vector<Node*> nodes = {const_cast<atoms::Node*>(node)};
    auto handles = add_nodes(nodes, throw_if_exists);
    return handles.empty() ? "" : handles[0];
}

string RemoteAtomDBPeer::add_link(const atoms::Link* link, bool throw_if_exists) {
    vector<Link*> links = {const_cast<atoms::Link*>(link)};
    auto handles = add_links(links, throw_if_exists);
    return handles.empty() ? "" : handles[0];
}

// Writes hold peer_mutex_ for the whole operation so the cache write, the staged_handles_
// update and any concurrent release_cache() swap stay mutually consistent (a write must land
// in the same cache generation as its staged handle). Cache ops are pure in-memory and fast.

vector<string> RemoteAtomDBPeer::add_atoms(const vector<atoms::Atom*>& atoms,
                                           bool throw_if_exists,
                                           bool is_transactional) {
    if (is_readonly()) return vector<string>();

    lock_guard<mutex> lock(peer_mutex_);
    fetched_link_templates_ = make_unique<HandleTrie>(HANDLE_HASH_SIZE - 1);
    auto handles = cache_->add_atoms(atoms, throw_if_exists, is_transactional);
    staged_handles_.insert(handles.begin(), handles.end());
    return handles;
}

vector<string> RemoteAtomDBPeer::add_nodes(const vector<atoms::Node*>& nodes,
                                           bool throw_if_exists,
                                           bool is_transactional) {
    if (is_readonly()) return vector<string>();

    lock_guard<mutex> lock(peer_mutex_);
    fetched_link_templates_ = make_unique<HandleTrie>(HANDLE_HASH_SIZE - 1);
    auto handles = cache_->add_nodes(nodes, throw_if_exists, is_transactional);
    staged_handles_.insert(handles.begin(), handles.end());
    return handles;
}

vector<string> RemoteAtomDBPeer::add_links(const vector<atoms::Link*>& links,
                                           bool throw_if_exists,
                                           bool is_transactional) {
    if (is_readonly()) return vector<string>();

    lock_guard<mutex> lock(peer_mutex_);
    fetched_link_templates_ = make_unique<HandleTrie>(HANDLE_HASH_SIZE - 1);
    auto handles = cache_->add_links(links, throw_if_exists, is_transactional);
    staged_handles_.insert(handles.begin(), handles.end());
    return handles;
}

bool RemoteAtomDBPeer::delete_atom(const string& handle, bool delete_link_targets) {
    if (is_readonly()) return false;

    bool cache_ok;
    {
        lock_guard<mutex> lock(peer_mutex_);
        cache_ok = cache_->delete_atom(handle, delete_link_targets);
        if (cache_ok) staged_handles_.erase(handle);
    }
    bool local_ok = local_persistence_->delete_atom(handle, delete_link_targets);
    if (cache_ok || local_ok) {
        lock_guard<mutex> lock(peer_mutex_);
        fetched_link_templates_ = make_unique<HandleTrie>(HANDLE_HASH_SIZE - 1);
    }
    return cache_ok || local_ok;
}

bool RemoteAtomDBPeer::delete_node(const string& handle, bool delete_link_targets) {
    if (is_readonly()) return false;

    bool cache_ok;
    {
        lock_guard<mutex> lock(peer_mutex_);
        cache_ok = cache_->delete_node(handle, delete_link_targets);
        if (cache_ok) staged_handles_.erase(handle);
    }
    bool local_ok = local_persistence_->delete_node(handle, delete_link_targets);
    if (cache_ok || local_ok) {
        lock_guard<mutex> lock(peer_mutex_);
        fetched_link_templates_ = make_unique<HandleTrie>(HANDLE_HASH_SIZE - 1);
    }
    return cache_ok || local_ok;
}

bool RemoteAtomDBPeer::delete_link(const string& handle, bool delete_link_targets) {
    if (is_readonly()) return false;

    bool cache_ok;
    {
        lock_guard<mutex> lock(peer_mutex_);
        cache_ok = cache_->delete_link(handle, delete_link_targets);
        if (cache_ok) staged_handles_.erase(handle);
    }
    bool local_ok = local_persistence_->delete_link(handle, delete_link_targets);
    if (cache_ok || local_ok) {
        lock_guard<mutex> lock(peer_mutex_);
        fetched_link_templates_ = make_unique<HandleTrie>(HANDLE_HASH_SIZE - 1);
    }
    return cache_ok || local_ok;
}

uint RemoteAtomDBPeer::delete_atoms(const vector<string>& handles, bool delete_link_targets) {
    if (is_readonly()) return 0;

    uint cache_count;
    {
        lock_guard<mutex> lock(peer_mutex_);
        cache_count = cache_->delete_atoms(handles, delete_link_targets);
        if (cache_count > 0) {
            for (const auto& h : handles) staged_handles_.erase(h);
        }
    }
    uint local_count = local_persistence_->delete_atoms(handles, delete_link_targets);
    if (cache_count > 0 || local_count > 0) {
        lock_guard<mutex> lock(peer_mutex_);
        fetched_link_templates_ = make_unique<HandleTrie>(HANDLE_HASH_SIZE - 1);
    }
    return cache_count + local_count;
}

uint RemoteAtomDBPeer::delete_nodes(const vector<string>& handles, bool delete_link_targets) {
    if (is_readonly()) return 0;

    uint cache_count;
    {
        lock_guard<mutex> lock(peer_mutex_);
        cache_count = cache_->delete_nodes(handles, delete_link_targets);
        if (cache_count > 0) {
            for (const auto& h : handles) staged_handles_.erase(h);
        }
    }
    uint local_count = local_persistence_->delete_nodes(handles, delete_link_targets);
    if (cache_count > 0 || local_count > 0) {
        lock_guard<mutex> lock(peer_mutex_);
        fetched_link_templates_ = make_unique<HandleTrie>(HANDLE_HASH_SIZE - 1);
    }
    return cache_count + local_count;
}

uint RemoteAtomDBPeer::delete_links(const vector<string>& handles, bool delete_link_targets) {
    if (is_readonly()) return 0;

    uint cache_count;
    {
        lock_guard<mutex> lock(peer_mutex_);
        cache_count = cache_->delete_links(handles, delete_link_targets);
        if (cache_count > 0) {
            for (const auto& h : handles) staged_handles_.erase(h);
        }
    }
    uint local_count = local_persistence_->delete_links(handles, delete_link_targets);
    if (cache_count > 0 || local_count > 0) {
        lock_guard<mutex> lock(peer_mutex_);
        fetched_link_templates_ = make_unique<HandleTrie>(HANDLE_HASH_SIZE - 1);
    }
    return cache_count + local_count;
}

void RemoteAtomDBPeer::re_index_patterns(bool flush_patterns) {
    if (is_readonly()) return;

    cache()->re_index_patterns(flush_patterns);
    if (local_persistence_) {
        local_persistence_->re_index_patterns(flush_patterns);
    }
}

size_t RemoteAtomDBPeer::node_count() const {
    size_t count = cache()->node_count();
    if (local_persistence_) {
        count += local_persistence_->node_count();
    }
    return count;
}

size_t RemoteAtomDBPeer::link_count() const {
    size_t count = cache()->link_count();
    if (local_persistence_) {
        count += local_persistence_->link_count();
    }
    return count;
}

size_t RemoteAtomDBPeer::atom_count() const {
    size_t count = cache()->atom_count();
    if (local_persistence_) {
        count += local_persistence_->atom_count();
    }
    return count;
}

void RemoteAtomDBPeer::fetch(const LinkSchema& link_schema) {
    {
        lock_guard<mutex> lock(peer_mutex_);
        if (fetched_link_templates_->lookup(link_schema.handle()) != nullptr) {
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
    if (fetched_link_templates_->lookup(link_schema.handle()) == nullptr) {
        fetched_link_templates_->insert(link_schema.handle(), new EmptyTrieValue());
    }
}

void RemoteAtomDBPeer::persist_atoms_to_local(const vector<shared_ptr<Atom>>& atoms) {
    if (!local_persistence_ || atoms.empty()) {
        return;
    }

    // Build dependency closure from this peer's own backends only (cache already flushed into
    // `atoms`, plus atomdb_ / local_persistence). Cross-peer federation is intentionally not
    // attempted here; local_persistence is expected to run with composite_type_enabled=false so
    // links can be written without resolving foreign targets for composite_type_hash.
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
    // would wipe large parts of the base KB when a staged handle is a shared node.
    if (!nodes.empty()) {
        local_persistence_->add_nodes(nodes, false, false);
    }

    if (links.empty()) {
        return;
    }

    // When composite_type is disabled, backends skip target-existence checks — persist links
    // directly even if some targets live only on other peers.
    if (!local_persistence_->composite_type_enabled()) {
        local_persistence_->add_links(links, false, false);
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
        local_persistence_->add_links(batch, false, false);
        remaining_links = pending;
    }
}

void RemoteAtomDBPeer::release_cache(bool persist_to_local, bool persist_entire_cache) {
    vector<shared_ptr<Atom>> to_persist;

    {
        lock_guard<mutex> lock(peer_mutex_);
        if (cache_->atom_count() == 0 && fetched_link_templates_->size == 0) {
            return;
        }

        LOG_DEBUG("[RemoteDB(" << uid_ << ")] release_cache: clearing " << cache_->atom_count()
                               << " cached atoms"
                               << (persist_entire_cache ? " (persist entire cache)"
                                                        : " (persist staged only)"));

        if (local_persistence_ && cache_->atom_count() > 0) {
            // Invariant: staged writes are never dropped. Even a "no persistence" release
            // flushes them, so a write staged between release()'s decision and this critical
            // section cannot be lost.
            if (persist_to_local && persist_entire_cache) {
                to_persist = cache_->get_all_atoms();
            } else if (!staged_handles_.empty()) {
                for (const auto& atom : cache_->get_all_atoms()) {
                    if (staged_handles_.count(atom->handle()) > 0) {
                        to_persist.push_back(atom);
                    }
                }
            }
        }

        // Swap in a fresh cache; concurrent readers holding the old snapshot keep a live,
        // internally thread-safe InMemoryDB until they drop it.
        cache_ = make_shared<InMemoryDB>(uid_);
        staged_handles_.clear();
        fetched_link_templates_ = make_unique<HandleTrie>(HANDLE_HASH_SIZE - 1);
    }

    if (!to_persist.empty()) {
        persist_atoms_to_local(to_persist);
    }
}

void RemoteAtomDBPeer::release(const LinkSchema& link_schema, bool persist, bool force) {
    bool schema_cached;
    bool has_staged_writes;
    bool has_cached_atoms;
    {
        lock_guard<mutex> lock(peer_mutex_);
        schema_cached = fetched_link_templates_->lookup(link_schema.handle()) != nullptr;
        has_staged_writes = !staged_handles_.empty();
        has_cached_atoms = cache_->atom_count() > 0;
    }

    // This snapshot is advisory: state may change before release_cache() re-locks. That is safe
    // because release_cache() never drops staged writes (see invariant there).
    if (persist) {
        if (schema_cached || force) {
            LOG_INFO("[RemoteDB(" << uid_ << ")] release(" << link_schema.handle()
                                  << ") clearing cache with persistence (or forced)");
            release_cache(true, true);
            return;
        }
        if (has_staged_writes) {
            LOG_INFO("[RemoteDB(" << uid_ << ")] release(" << link_schema.handle()
                                  << ") persisting staged cache writes");
            release_cache(true, false);
            return;
        }
        if (!is_readonly() && has_cached_atoms) {
            // Important for memory pressure: callers may request a flush even when this exact
            // schema token is not marked as fetched anymore.
            LOG_INFO("[RemoteDB(" << uid_ << ")] release(" << link_schema.handle()
                                  << ") clearing read cache without persistence");
            release_cache(false, true);
            return;
        }
        LOG_DEBUG(
            "[RemoteDB(" << uid_ << ")] release(" << link_schema.handle()
                         << ") no-op (schema not cached, no staged writes, empty cache or readonly)");
        return;
    }

    if (schema_cached) {
        lock_guard<mutex> lock(peer_mutex_);
        fetched_link_templates_->remove(link_schema.handle());
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
        release_cache(true, true);
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
