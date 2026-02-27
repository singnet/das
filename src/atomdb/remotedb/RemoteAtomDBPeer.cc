#include "RemoteAtomDBPeer.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "Atom.h"
#include "InMemoryDBAPITypes.h"
#include "Link.h"
#include "Node.h"
#include "Utils.h"
#include "expression_hasher.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace atomdb_api_types;
using namespace atoms;
using namespace commons;

// Dummy TrieValue for using HandleTrie as a set (presence only)
class EmptyTrieValue : public HandleTrie::TrieValue {
   public:
    void merge(HandleTrie::TrieValue* /*other*/) override {}
};

RemoteAtomDBPeer::RemoteAtomDBPeer(shared_ptr<AtomDB> remote_atomdb,
                                   shared_ptr<AtomDB> local_persistence,
                                   const string& uid)
    : uid_(uid),
      cache_(uid),
      atomdb_(remote_atomdb),
      local_persistence_(local_persistence),
      fetched_link_templates_(HANDLE_HASH_SIZE - 1) {
    start_cleanup_thread();
}

RemoteAtomDBPeer::~RemoteAtomDBPeer() { stop_cleanup_thread(); }

bool RemoteAtomDBPeer::allow_nested_indexing() { return false; }

shared_ptr<Atom> RemoteAtomDBPeer::get_atom(const string& handle) {
    auto atom = cache_.get_atom(handle);
    if (atom) return atom;

    if (local_persistence_) {
        atom = local_persistence_->get_atom(handle);
        if (atom) {
            cache_.add_atom(atom.get());
            return atom;
        }
    }

    if (atomdb_) {
        atom = atomdb_->get_atom(handle);
        if (atom) {
            cache_.add_atom(atom.get());
            return atom;
        }
    }

    return nullptr;
}

shared_ptr<Node> RemoteAtomDBPeer::get_node(const string& handle) {
    auto node = cache_.get_node(handle);
    if (node) return node;

    if (local_persistence_) {
        node = local_persistence_->get_node(handle);
        if (node) {
            cache_.add_node(node.get());
            return node;
        }
    }

    if (atomdb_) {
        node = atomdb_->get_node(handle);
        if (node) {
            cache_.add_node(node.get());
            return node;
        }
    }

    return nullptr;
}

shared_ptr<Link> RemoteAtomDBPeer::get_link(const string& handle) {
    auto link = cache_.get_link(handle);
    if (link) return link;

    if (local_persistence_) {
        link = local_persistence_->get_link(handle);
        if (link) {
            cache_.add_link(link.get());
            return link;
        }
    }

    if (atomdb_) {
        link = atomdb_->get_link(handle);
        if (link) {
            cache_.add_link(link.get());
            return link;
        }
    }

    return nullptr;
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

    merge_results(cache_.get_matching_atoms(is_toplevel, key));
    if (local_persistence_) {
        merge_results(local_persistence_->get_matching_atoms(is_toplevel, key));
    }

    if (!local_only && atomdb_) {
        merge_results(atomdb_->get_matching_atoms(is_toplevel, key));
    }

    return result;
}

bool RemoteAtomDBPeer::schema_already_fetched(const LinkSchema& link_schema) {
    return fetched_link_templates_.lookup(link_schema.handle()) != nullptr;
}

void RemoteAtomDBPeer::feed_cache_from_handle_set(shared_ptr<HandleSet> handle_set) {
    if (!handle_set) return;

    auto it = handle_set->get_iterator();
    if (!it) return;

    while (true) {
        char* handle_cstr = it->next();
        if (!handle_cstr) break;

        string handle(handle_cstr);
        auto atom = get_atom(handle);
        if (atom) {
            cache_.add_atom(atom.get());
        }
    }
}

shared_ptr<HandleSet> RemoteAtomDBPeer::query_for_pattern(const LinkSchema& link_schema) {
    auto result = make_shared<HandleSetInMemory>();
    set<string> all_handles;

    auto collect_handles = [&](shared_ptr<HandleSet> handle_set) {
        if (!handle_set) return;
        auto it = handle_set->get_iterator();
        char* handle;
        while ((handle = it->next()) != nullptr) {
            string h(handle);
            if (all_handles.insert(h).second) {
                result->add_handle(h);
            }
        }
    };

    if (schema_already_fetched(link_schema)) {
        collect_handles(cache_.query_for_pattern(link_schema));
        if (local_persistence_) {
            collect_handles(local_persistence_->query_for_pattern(link_schema));
        }
    } else {
        if (atomdb_) {
            collect_handles(atomdb_->query_for_pattern(link_schema));
        }
        feed_cache_from_handle_set(result);
        fetched_link_templates_.insert(link_schema.handle(), new EmptyTrieValue());
    }

    return result;
}

shared_ptr<HandleList> RemoteAtomDBPeer::query_for_targets(const string& handle) {
    auto result = cache_.query_for_targets(handle);
    if (result) return result;

    if (local_persistence_) {
        result = local_persistence_->query_for_targets(handle);
        if (result) return result;
    }

    if (atomdb_) {
        return atomdb_->query_for_targets(handle);
    }

    return nullptr;
}

shared_ptr<HandleSet> RemoteAtomDBPeer::query_for_incoming_set(const string& handle) {
    auto result = cache_.query_for_incoming_set(handle);
    if (result && result->size() > 0) return result;

    if (local_persistence_) {
        auto local_result = local_persistence_->query_for_incoming_set(handle);
        if (local_result && local_result->size() > 0) return local_result;
    }

    if (atomdb_) {
        return atomdb_->query_for_incoming_set(handle);
    }

    return make_shared<HandleSetInMemory>();
}

bool RemoteAtomDBPeer::atom_exists(const string& handle) {
    if (cache_.atom_exists(handle)) return true;
    if (local_persistence_ && local_persistence_->atom_exists(handle)) return true;
    if (atomdb_ && atomdb_->atom_exists(handle)) return true;
    return false;
}

bool RemoteAtomDBPeer::node_exists(const string& handle) {
    if (cache_.node_exists(handle)) return true;
    if (local_persistence_ && local_persistence_->node_exists(handle)) return true;
    if (atomdb_ && atomdb_->node_exists(handle)) return true;
    return false;
}

bool RemoteAtomDBPeer::link_exists(const string& handle) {
    if (cache_.link_exists(handle)) return true;
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
        auto found = db.atoms_exist(to_check);
        for (const auto& h : found) {
            result.insert(h);
            remaining.erase(h);
        }
    };

    from_source(cache_);
    if (local_persistence_ && !remaining.empty()) {
        from_source(*local_persistence_);
    }
    if (atomdb_ && !remaining.empty()) {
        from_source(*atomdb_);
    }

    return result;
}

set<string> RemoteAtomDBPeer::nodes_exist(const vector<string>& handles) {
    set<string> result;
    set<string> remaining(handles.begin(), handles.end());

    auto from_source = [&](AtomDB& db) {
        vector<string> to_check(remaining.begin(), remaining.end());
        if (to_check.empty()) return;
        auto found = db.nodes_exist(to_check);
        for (const auto& h : found) {
            result.insert(h);
            remaining.erase(h);
        }
    };

    from_source(cache_);
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
        auto found = db.links_exist(to_check);
        for (const auto& h : found) {
            result.insert(h);
            remaining.erase(h);
        }
    };

    from_source(cache_);
    if (local_persistence_ && !remaining.empty()) from_source(*local_persistence_);
    if (atomdb_ && !remaining.empty()) from_source(*atomdb_);

    return result;
}

string RemoteAtomDBPeer::add_atom(const atoms::Atom* atom, bool throw_if_exists) {
    return cache_.add_atom(atom, throw_if_exists);
}

string RemoteAtomDBPeer::add_node(const atoms::Node* node, bool throw_if_exists) {
    return cache_.add_node(node, throw_if_exists);
}

string RemoteAtomDBPeer::add_link(const atoms::Link* link, bool throw_if_exists) {
    return cache_.add_link(link, throw_if_exists);
}

vector<string> RemoteAtomDBPeer::add_atoms(const vector<atoms::Atom*>& atoms,
                                           bool throw_if_exists,
                                           bool is_transactional) {
    return cache_.add_atoms(atoms, throw_if_exists, is_transactional);
}

vector<string> RemoteAtomDBPeer::add_nodes(const vector<atoms::Node*>& nodes,
                                           bool throw_if_exists,
                                           bool is_transactional) {
    return cache_.add_nodes(nodes, throw_if_exists, is_transactional);
}

vector<string> RemoteAtomDBPeer::add_links(const vector<atoms::Link*>& links,
                                           bool throw_if_exists,
                                           bool is_transactional) {
    return cache_.add_links(links, throw_if_exists, is_transactional);
}

bool RemoteAtomDBPeer::delete_atom(const string& handle, bool delete_link_targets) {
    bool cache_ok = cache_.delete_atom(handle, delete_link_targets);
    bool local_ok = true;
    if (local_persistence_) {
        local_ok = local_persistence_->delete_atom(handle, delete_link_targets);
    }
    return cache_ok || local_ok;
}

bool RemoteAtomDBPeer::delete_node(const string& handle, bool delete_link_targets) {
    bool cache_ok = cache_.delete_node(handle, delete_link_targets);
    bool local_ok = true;
    if (local_persistence_) {
        local_ok = local_persistence_->delete_node(handle, delete_link_targets);
    }
    return cache_ok || local_ok;
}

bool RemoteAtomDBPeer::delete_link(const string& handle, bool delete_link_targets) {
    bool cache_ok = cache_.delete_link(handle, delete_link_targets);
    bool local_ok = true;
    if (local_persistence_) {
        local_ok = local_persistence_->delete_link(handle, delete_link_targets);
    }
    return cache_ok || local_ok;
}

uint RemoteAtomDBPeer::delete_atoms(const vector<string>& handles, bool delete_link_targets) {
    uint cache_count = cache_.delete_atoms(handles, delete_link_targets);
    uint local_count = 0;
    if (local_persistence_) {
        local_count = local_persistence_->delete_atoms(handles, delete_link_targets);
    }
    return cache_count + local_count;
}

uint RemoteAtomDBPeer::delete_nodes(const vector<string>& handles, bool delete_link_targets) {
    uint cache_count = cache_.delete_nodes(handles, delete_link_targets);
    uint local_count = 0;
    if (local_persistence_) {
        local_count = local_persistence_->delete_nodes(handles, delete_link_targets);
    }
    return cache_count + local_count;
}

uint RemoteAtomDBPeer::delete_links(const vector<string>& handles, bool delete_link_targets) {
    uint cache_count = cache_.delete_links(handles, delete_link_targets);
    uint local_count = 0;
    if (local_persistence_) {
        local_count = local_persistence_->delete_links(handles, delete_link_targets);
    }
    return cache_count + local_count;
}

void RemoteAtomDBPeer::re_index_patterns(bool flush_patterns) {
    cache_.re_index_patterns(flush_patterns);
    if (local_persistence_) {
        local_persistence_->re_index_patterns(flush_patterns);
    }
}

void RemoteAtomDBPeer::fetch(const LinkSchema& link_schema) {
    if (!schema_already_fetched(link_schema) && atomdb_) {
        auto result = atomdb_->query_for_pattern(link_schema);
        if (result) {
            feed_cache_from_handle_set(result);
            fetched_link_templates_.insert(link_schema.handle(), new EmptyTrieValue());
        }
    }
}

void RemoteAtomDBPeer::release(const LinkSchema& link_schema) {
    // Query cache for pattern results before removing, so we can persist them.
    // TODO: This may remove atoms that were also fetched by another schema. We need to handle this
    // properly.
    auto handle_set = cache_.query_for_pattern(link_schema);
    if (handle_set && local_persistence_) {
        auto it = handle_set->get_iterator();
        char* handle_cstr;
        while ((handle_cstr = it->next()) != nullptr) {
            string handle(handle_cstr);
            auto atom = cache_.get_atom(handle);
            if (atom) {
                local_persistence_->add_atom(atom.get(), false);
                cache_.delete_atom(handle, false);
            }
        }
    }
    fetched_link_templates_.remove(link_schema.handle());
}

double RemoteAtomDBPeer::available_ram() {
    unsigned long total = Utils::get_total_ram();
    if (total == 0) return 0.0;
    return static_cast<double>(Utils::get_current_free_ram()) / static_cast<double>(total);
}

// TODO: Implement actual cache eviction
void RemoteAtomDBPeer::auto_cleanup() {
    double ram_fraction = available_ram();
    if (ram_fraction < CRITICAL_RAM_THRESHOLD) {
        LOG_INFO("RemoteAtomDBPeer " << uid_ << ": Low RAM detected (available="
                                     << (ram_fraction * 100.0) << "%), cache cleanup triggered");
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
