#define LOG_LEVEL INFO_LEVEL
#include "RemoteAtomDB.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <utility>

#include "InMemoryDB.h"
#include "InMemoryDBAPITypes.h"
#include "Link.h"
#include "Logger.h"
#include "MorkDB.h"
#include "Node.h"
#include "RedisMongoDB.h"
#include "Utils.h"

using namespace atomdb;
using namespace atoms;
using namespace commons;

using json = nlohmann::json;

RemoteAtomDB::RemoteAtomDB(map<string, shared_ptr<RemoteAtomDBPeer>> peers)
    : remote_db_(std::move(peers)) {
    LOG_INFO("RemoteAtomDB initialized with " << remote_db_.size() << " pre-built peers");
    finalize_peer_lists();
}

RemoteAtomDB::~RemoteAtomDB() = default;

atomdb_api_types::ProtectionMode RemoteAtomDB::get_protection_mode() const {
    for (auto& [uid, peer] : remote_db_) {
        if (peer->get_protection_mode() != atomdb_api_types::ProtectionMode::UNPROTECTED) {
            return atomdb_api_types::ProtectionMode::FORWARD;
        }
    }
    return atomdb_api_types::ProtectionMode::UNPROTECTED;
}

void RemoteAtomDB::finalize_peer_lists() {
    writable_peers_.clear();
    readonly_peers_.clear();
    writable_peers_.reserve(remote_db_.size());
    readonly_peers_.reserve(remote_db_.size());

    unsigned int nested_peers = 0;
    for (auto& [uid, peer] : remote_db_) {
        if (peer->is_readonly()) {
            readonly_peers_.emplace_back(uid, peer);
        } else {
            writable_peers_.emplace_back(uid, peer);
        }
        if (peer->allow_nested_indexing()) nested_peers++;
    }

    // Derive aggregated nested-indexing. A single global boolean cannot describe a
    // heterogeneous result set, so mixed configs are normalized to false.
    if (!remote_db_.empty() && nested_peers == remote_db_.size()) {
        nested_indexing_ = true;
    } else {
        nested_indexing_ = false;
        if (nested_peers > 0) {
            LOG_INFO(
                "WARNING: RemoteAtomDB has a mix of nested-indexing and non-nested-indexing "
                "peers ("
                << nested_peers << "/" << remote_db_.size()
                << " nested); downgrading allow_nested_indexing() to false. Nested peers will "
                   "be re-matched locally by the query engine.");
        }
    }
}

bool RemoteAtomDB::composite_type_enabled() const {
    LOG_DEBUG(
        "RemoteAtomDB derives composite_type_enabled() from peers (true if any peer has it enabled)");
    for (auto& [uid, peer] : remote_db_) {
        if (peer->composite_type_enabled()) {
            return true;
        }
    }
    return false;
}

bool RemoteAtomDB::allow_nested_indexing() { return nested_indexing_; }

shared_ptr<Atom> RemoteAtomDB::get_atom(const string& handle) {
    // Writable peers first: their write buffer / local_persistence are the source of truth
    // for updated custom attributes (strength) that share a content-addressed handle.
    for (auto& [uid, peer] : writable_peers_) {
        auto atom = peer->get_atom(handle);
        if (atom) {
            LOG_DEBUG("get_atom(" << handle << ") fetched from writable peer [" << uid << "]");
            return atom;
        }
    }

    // Readonly peers: cache probe then escalate to remote backends (base KB hot path).
    for (auto& [uid, peer] : readonly_peers_) {
        auto atom = peer->get_cached_atom(handle);
        if (atom) return atom;
    }
    for (auto& [uid, peer] : readonly_peers_) {
        auto atom = peer->get_atom(handle);
        if (atom) {
            LOG_DEBUG("get_atom(" << handle << ") fetched from [" << uid << "]");
            return atom;
        }
    }
    LOG_DEBUG("get_atom(" << handle << ") not found in any peer");
    return nullptr;
}

shared_ptr<Node> RemoteAtomDB::get_node(const string& handle) {
    auto atom = get_atom(handle);
    return dynamic_pointer_cast<Node>(atom);
}

shared_ptr<Link> RemoteAtomDB::get_link(const string& handle) {
    auto atom = get_atom(handle);
    return dynamic_pointer_cast<Link>(atom);
}

vector<shared_ptr<Atom>> RemoteAtomDB::get_matching_atoms(bool is_toplevel, Atom& key) {
    vector<shared_ptr<Atom>> result;
    set<string> seen;

    for (auto& [uid, peer] : remote_db_) {
        auto atoms = peer->get_matching_atoms(is_toplevel, key);
        for (const auto& atom : atoms) {
            string h = atom->handle();
            if (seen.insert(h).second) {
                result.push_back(atom);
            }
        }
    }
    return result;
}

shared_ptr<atomdb_api_types::HandleSet> RemoteAtomDB::query_for_pattern(const LinkSchema& link_schema) {
    auto result = make_shared<atomdb_api_types::HandleSetInMemory>();
    set<string> seen;

    LOG_DEBUG("query_for_pattern(" << link_schema.handle() << ") fan-out to " << remote_db_.size()
                                   << " peers");
    for (auto& [uid, peer] : remote_db_) {
        auto handle_set = peer->query_for_pattern(link_schema);
        if (!handle_set) continue;

        // Preserve per-handle assignments / metta expressions for nested-indexing peers so the
        // aggregated result stays faithful instead of silently dropping the backend's match data.
        bool copy_metadata = peer->allow_nested_indexing();
        LOG_DEBUG("  [" << uid << "] returned " << handle_set->size() << " handles"
                        << (copy_metadata ? " (with metadata)" : ""));

        auto it = handle_set->get_iterator();
        if (!it) continue;

        while (true) {
            char* h = it->next();
            if (!h) break;
            string handle(h);
            if (seen.insert(handle).second) {
                if (copy_metadata) {
                    result->add_handle(handle,
                                       handle_set->get_metta_expressions_by_handle(handle),
                                       handle_set->get_assignments_by_handle(handle));
                } else {
                    result->add_handle(handle);
                }
            }
        }
    }
    LOG_DEBUG("query_for_pattern(" << link_schema.handle() << ") aggregated " << result->size()
                                   << " unique handles");
    return result;
}

shared_ptr<atomdb_api_types::HandleList> RemoteAtomDB::query_for_targets(const string& handle) {
    for (auto& [uid, peer] : remote_db_) {
        auto list = peer->query_for_targets(handle);
        if (list) {
            LOG_DEBUG("query_for_targets(" << handle << ") served by peer [" << uid << "]");
            return list;
        }
    }
    LOG_DEBUG("query_for_targets(" << handle << ") not found in any peer");
    return nullptr;
}

shared_ptr<atomdb_api_types::HandleSet> RemoteAtomDB::query_for_incoming_set(const string& handle) {
    auto result = make_shared<atomdb_api_types::HandleSetInMemory>();
    set<string> seen;

    LOG_DEBUG("query_for_incoming_set(" << handle << ") fan-out to " << remote_db_.size() << " peers");
    for (auto& [uid, peer] : remote_db_) {
        auto handle_set = peer->query_for_incoming_set(handle);
        if (!handle_set) continue;

        auto it = handle_set->get_iterator();
        if (!it) continue;

        while (true) {
            char* h = it->next();
            if (!h) break;
            string member(h);
            if (seen.insert(member).second) {
                result->add_handle(member);
            }
        }
    }
    LOG_DEBUG("query_for_incoming_set(" << handle << ") aggregated " << result->size()
                                        << " unique handles");
    return result;
}

bool RemoteAtomDB::atom_exists(const string& handle) {
    for (auto& [uid, peer] : remote_db_) {
        if (peer->atom_exists(handle)) return true;
    }
    return false;
}

bool RemoteAtomDB::node_exists(const string& handle) {
    for (auto& [uid, peer] : remote_db_) {
        if (peer->node_exists(handle)) return true;
    }
    return false;
}

bool RemoteAtomDB::link_exists(const string& handle) {
    for (auto& [uid, peer] : remote_db_) {
        if (peer->link_exists(handle)) return true;
    }
    return false;
}

namespace {

template <typename PeerExistFn>
set<string> fanout_exist(const map<string, shared_ptr<RemoteAtomDBPeer>>& peers,
                         const vector<string>& handles,
                         PeerExistFn peer_exist) {
    set<string> result;
    set<string> remaining(handles.begin(), handles.end());

    for (auto& [uid, peer] : peers) {
        if (remaining.empty()) break;
        vector<string> to_check(remaining.begin(), remaining.end());
        auto found = peer_exist(*peer, to_check);
        for (const auto& h : found) {
            result.insert(h);
            remaining.erase(h);
        }
    }
    return result;
}

}  // namespace

set<string> RemoteAtomDB::atoms_exist(const vector<string>& handles) {
    return fanout_exist(remote_db_, handles, [](RemoteAtomDBPeer& p, const vector<string>& h) {
        return p.atoms_exist(h);
    });
}

set<string> RemoteAtomDB::nodes_exist(const vector<string>& handles) {
    return fanout_exist(remote_db_, handles, [](RemoteAtomDBPeer& p, const vector<string>& h) {
        return p.nodes_exist(h);
    });
}

set<string> RemoteAtomDB::links_exist(const vector<string>& handles) {
    return fanout_exist(remote_db_, handles, [](RemoteAtomDBPeer& p, const vector<string>& h) {
        return p.links_exist(h);
    });
}

string RemoteAtomDB::add_atom(const atoms::Atom* atom, const atoms::Merger* merger) {
    // Writes only land on writable peers (readonly peers gate the call internally).
    string handle;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("add_atom(" << atom->handle() << ") to peer [" << uid << "]");
        string peer_handle = peer->add_atom(atom, merger);
        if (!peer_handle.empty()) handle = peer_handle;
    }
    return handle;
}

string RemoteAtomDB::add_node(const atoms::Node* node, const atoms::Merger* merger) {
    string handle;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("add_node(" << node->handle() << ") to peer [" << uid << "]");
        string peer_handle = peer->add_node(node, merger);
        if (!peer_handle.empty()) handle = peer_handle;
    }
    return handle;
}

string RemoteAtomDB::add_link(const atoms::Link* link, const atoms::Merger* merger) {
    string handle;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("add_link(" << link->handle() << ") to peer [" << uid << "]");
        string peer_handle = peer->add_link(link, merger);
        if (!peer_handle.empty()) handle = peer_handle;
    }
    return handle;
}

vector<string> RemoteAtomDB::add_atoms(const vector<atoms::Atom*>& atoms,
                                       bool is_transactional,
                                       const atoms::Merger* merger) {
    vector<string> handles;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("add_atoms(" << atoms.size() << ") to peer [" << uid << "]");
        auto peer_handles = peer->add_atoms(atoms, is_transactional, merger);
        if (!peer_handles.empty()) handles = peer_handles;
    }
    return handles;
}

vector<string> RemoteAtomDB::add_nodes(const vector<atoms::Node*>& nodes,
                                       bool is_transactional,
                                       const atoms::Merger* merger) {
    vector<string> handles;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("add_nodes(" << nodes.size() << ") to peer [" << uid << "]");
        auto peer_handles = peer->add_nodes(nodes, is_transactional, merger);
        if (!peer_handles.empty()) handles = peer_handles;
    }
    return handles;
}

vector<string> RemoteAtomDB::add_links(const vector<atoms::Link*>& links,
                                       bool is_transactional,
                                       const atoms::Merger* merger) {
    vector<string> handles;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("add_links(" << links.size() << ") to peer [" << uid << "]");
        auto peer_handles = peer->add_links(links, is_transactional, merger);
        if (!peer_handles.empty()) handles = peer_handles;
    }
    return handles;
}

bool RemoteAtomDB::delete_atom(const string& handle, bool delete_link_targets) {
    bool ok = false;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("delete_atom(" << handle << ") from peer [" << uid << "]");
        ok = peer->delete_atom(handle, delete_link_targets) || ok;
    }
    return ok;
}

bool RemoteAtomDB::delete_node(const string& handle, bool delete_link_targets) {
    bool ok = false;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("delete_node(" << handle << ") from peer [" << uid << "]");
        ok = peer->delete_node(handle, delete_link_targets) || ok;
    }
    return ok;
}

bool RemoteAtomDB::delete_link(const string& handle, bool delete_link_targets) {
    bool ok = false;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("delete_link(" << handle << ") from peer [" << uid << "]");
        ok = peer->delete_link(handle, delete_link_targets) || ok;
    }
    return ok;
}

uint RemoteAtomDB::delete_atoms(const vector<string>& handles, bool delete_link_targets) {
    uint count = 0;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("delete_atoms(" << handles.size() << ") from peer [" << uid << "]");
        count = max(count, peer->delete_atoms(handles, delete_link_targets));
    }
    return count;
}

uint RemoteAtomDB::delete_nodes(const vector<string>& handles, bool delete_link_targets) {
    uint count = 0;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("delete_nodes(" << handles.size() << ") from peer [" << uid << "]");
        count = max(count, peer->delete_nodes(handles, delete_link_targets));
    }
    return count;
}

uint RemoteAtomDB::delete_links(const vector<string>& handles, bool delete_link_targets) {
    uint count = 0;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("delete_links(" << handles.size() << ") from peer [" << uid << "]");
        count = max(count, peer->delete_links(handles, delete_link_targets));
    }
    return count;
}

void RemoteAtomDB::re_index_patterns(bool flush_patterns) {
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("re_index_patterns(" << flush_patterns << ") from peer [" << uid << "]");
        peer->re_index_patterns(flush_patterns);
    }
}

size_t RemoteAtomDB::node_count() const {
    size_t count = 0;
    for (auto& [uid, peer] : remote_db_) {
        count += peer->node_count();
    }
    return count;
}

size_t RemoteAtomDB::link_count() const {
    size_t count = 0;
    for (auto& [uid, peer] : remote_db_) {
        count += peer->link_count();
    }
    return count;
}

size_t RemoteAtomDB::atom_count() const {
    size_t count = 0;
    for (auto& [uid, peer] : remote_db_) {
        count += peer->atom_count();
    }
    return count;
}

RemoteAtomDBPeer* RemoteAtomDB::get_peer(const string& uid) {
    auto it = remote_db_.find(uid);
    return (it != remote_db_.end()) ? it->second.get() : nullptr;
}

void RemoteAtomDB::release_caches(const LinkSchema& link_schema, bool persist, bool force) {
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("release_caches(" << link_schema.handle() << ") from peer [" << uid << "]");
        peer->release(link_schema, persist, force);
    }
}
