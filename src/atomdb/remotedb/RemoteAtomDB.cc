#define LOG_LEVEL INFO_LEVEL
#include "RemoteAtomDB.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <utility>

#include "InMemoryDB.h"
#include "InMemoryDBAPITypes.h"
#include "Logger.h"
#include "MorkDB.h"
#include "RedisMongoDB.h"
#include "Utils.h"

using namespace atomdb;
using namespace atoms;
using namespace commons;

using json = nlohmann::json;

namespace {

shared_ptr<AtomDB> create_atomdb_from_config(const JsonConfig& config) {
    string uid = config.at_path("uid").get_or<string>("");
    string type = config.at_path("type").get_or<string>("");
    string context = config.at_path("context").get_or<string>("");

    if (type == "inmemorydb") {
        return make_shared<InMemoryDB>(context.empty() ? "remotedb_" : context);
    }

    if (type == "redismongodb") {
        RedisMongoDB::initialize_statics(context);
        auto atomdb = make_shared<RedisMongoDB>(context, false, config);
        return atomdb;
    }

    if (type == "morkdb") {
        auto atomdb = make_shared<MorkDB>(context, config);
        return atomdb;
    }

    RAISE_ERROR("Unknown AtomDB type for peer " + uid + ": " + type);
    return nullptr;
}

}  // namespace

RemoteAtomDB::RemoteAtomDB(const JsonConfig& peers_config) {
    for (auto& entry : peers_config) {
        auto peer_config = JsonConfig(entry);
        string uid = peer_config.at_path("uid").get_or<string>("");
        if (uid.empty()) continue;

        shared_ptr<AtomDB> local_persistence = nullptr;
        auto local_persistence_config =
            peer_config.at_path("local_persistence").get_or<JsonConfig>(JsonConfig());
        if (!local_persistence_config.empty()) {
            local_persistence = create_atomdb_from_config(local_persistence_config);
        }
        remote_db_[uid] = make_shared<RemoteAtomDBPeer>(
            create_atomdb_from_config(peer_config), local_persistence, uid);
    }

    LOG_INFO("RemoteAtomDB initialized with " << remote_db_.size() << " remote peers");
    derive_nested_indexing();
}

RemoteAtomDB::RemoteAtomDB(map<string, shared_ptr<RemoteAtomDBPeer>> peers)
    : remote_db_(std::move(peers)) {
    LOG_INFO("RemoteAtomDB initialized with " << remote_db_.size() << " pre-built peers");
    derive_nested_indexing();
}

RemoteAtomDB::~RemoteAtomDB() = default;

void RemoteAtomDB::derive_nested_indexing() {
    // Derive the aggregated nested-indexing capability from the peers. A single global boolean
    // cannot describe a heterogeneous result set, so mixed configurations are normalized to the
    // lowest common denominator (false: the query engine re-matches every handle locally).
    unsigned int nested_peers = 0;
    for (auto& [uid, peer] : remote_db_) {
        if (peer->allow_nested_indexing()) nested_peers++;
    }
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

bool RemoteAtomDB::allow_nested_indexing() { return nested_indexing_; }

shared_ptr<Atom> RemoteAtomDB::get_atom(const string& handle) {
    // Phase 1: probe every peer's in-memory cache first (no network). Silent: this is the hot path.
    for (auto& [uid, peer] : remote_db_) {
        auto atom = peer->get_cached_atom(handle);
        if (atom) return atom;
    }
    // Phase 2: escalate to peers (local_persistence + remote backend) only when no cache has it.
    for (auto& [uid, peer] : remote_db_) {
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
    for (auto& [uid, peer] : remote_db_) {
        auto node = peer->get_cached_node(handle);
        if (node) return node;
    }
    for (auto& [uid, peer] : remote_db_) {
        auto node = peer->get_node(handle);
        if (node) {
            LOG_DEBUG("get_node(" << handle << ") fetched from [" << uid << "]");
            return node;
        }
    }
    LOG_DEBUG("get_node(" << handle << ") not found in any peer");
    return nullptr;
}

shared_ptr<Link> RemoteAtomDB::get_link(const string& handle) {
    for (auto& [uid, peer] : remote_db_) {
        auto link = peer->get_cached_link(handle);
        if (link) return link;
    }
    for (auto& [uid, peer] : remote_db_) {
        auto link = peer->get_link(handle);
        if (link) {
            LOG_DEBUG("get_link(" << handle << ") fetched from [" << uid << "]");
            return link;
        }
    }
    LOG_DEBUG("get_link(" << handle << ") not found in any peer");
    return nullptr;
}

vector<shared_ptr<Atom>> RemoteAtomDB::get_matching_atoms(bool is_toplevel, Atom& key) {
    vector<shared_ptr<Atom>> result;
    set<string> seen;

    for (auto& [uid, peer] : remote_db_) {
        auto atoms = peer->get_matching_atoms(is_toplevel, key);
        for (const auto& atom : atoms) {
            string h = atom->handle();
            if (seen.find(h) == seen.end()) {
                seen.insert(h);
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

set<string> RemoteAtomDB::atoms_exist(const vector<string>& handles) {
    set<string> result;
    set<string> remaining(handles.begin(), handles.end());

    for (auto& [uid, peer] : remote_db_) {
        if (remaining.empty()) break;
        vector<string> to_check(remaining.begin(), remaining.end());
        auto found = peer->atoms_exist(to_check);
        for (const auto& h : found) {
            result.insert(h);
            remaining.erase(h);
        }
    }
    return result;
}

set<string> RemoteAtomDB::nodes_exist(const vector<string>& handles) {
    set<string> result;
    set<string> remaining(handles.begin(), handles.end());

    for (auto& [uid, peer] : remote_db_) {
        if (remaining.empty()) break;
        vector<string> to_check(remaining.begin(), remaining.end());
        auto found = peer->nodes_exist(to_check);
        for (const auto& h : found) {
            result.insert(h);
            remaining.erase(h);
        }
    }
    return result;
}

set<string> RemoteAtomDB::links_exist(const vector<string>& handles) {
    set<string> result;
    set<string> remaining(handles.begin(), handles.end());

    for (auto& [uid, peer] : remote_db_) {
        if (remaining.empty()) break;
        vector<string> to_check(remaining.begin(), remaining.end());
        auto found = peer->links_exist(to_check);
        for (const auto& h : found) {
            result.insert(h);
            remaining.erase(h);
        }
    }
    return result;
}

string RemoteAtomDB::add_atom(const atoms::Atom* atom, bool throw_if_exists) {
    string handle;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("add_atom(" << atom->handle() << ") to peer [" << uid << "]");
        handle = peer->add_atom(atom, throw_if_exists);
    }
    return handle;
}

string RemoteAtomDB::add_node(const atoms::Node* node, bool throw_if_exists) {
    string handle;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("add_node(" << node->handle() << ") to peer [" << uid << "]");
        handle = peer->add_node(node, throw_if_exists);
    }
    return handle;
}

string RemoteAtomDB::add_link(const atoms::Link* link, bool throw_if_exists) {
    string handle;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("add_link(" << link->handle() << ") to peer [" << uid << "]");
        handle = peer->add_link(link, throw_if_exists);
    }
    return handle;
}

vector<string> RemoteAtomDB::add_atoms(const vector<atoms::Atom*>& atoms,
                                       bool throw_if_exists,
                                       bool is_transactional) {
    vector<string> handles;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("add_atoms(" << atoms.size() << ") to peer [" << uid << "]");
        handles = peer->add_atoms(atoms, throw_if_exists, is_transactional);
    }
    return handles;
}

vector<string> RemoteAtomDB::add_nodes(const vector<atoms::Node*>& nodes,
                                       bool throw_if_exists,
                                       bool is_transactional) {
    vector<string> handles;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("add_nodes(" << nodes.size() << ") to peer [" << uid << "]");
        handles = peer->add_nodes(nodes, throw_if_exists, is_transactional);
    }
    return handles;
}

vector<string> RemoteAtomDB::add_links(const vector<atoms::Link*>& links,
                                       bool throw_if_exists,
                                       bool is_transactional) {
    vector<string> handles;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("add_links(" << links.size() << ") to peer [" << uid << "]");
        handles = peer->add_links(links, throw_if_exists, is_transactional);
    }
    return handles;
}

bool RemoteAtomDB::delete_atom(const string& handle, bool delete_link_targets) {
    bool ok = true;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("delete_atom(" << handle << ") from peer [" << uid << "]");
        ok = peer->delete_atom(handle, delete_link_targets) && ok;
    }
    return ok;
}

bool RemoteAtomDB::delete_node(const string& handle, bool delete_link_targets) {
    bool ok = true;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("delete_node(" << handle << ") from peer [" << uid << "]");
        ok = peer->delete_node(handle, delete_link_targets) && ok;
    }
    return ok;
}

bool RemoteAtomDB::delete_link(const string& handle, bool delete_link_targets) {
    bool ok = true;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("delete_link(" << handle << ") from peer [" << uid << "]");
        ok = peer->delete_link(handle, delete_link_targets) && ok;
    }
    return ok;
}

uint RemoteAtomDB::delete_atoms(const vector<string>& handles, bool delete_link_targets) {
    uint count = 0;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("delete_atoms(" << handles.size() << ") from peer [" << uid << "]");
        count = peer->delete_atoms(handles, delete_link_targets);
    }
    return count;
}

uint RemoteAtomDB::delete_nodes(const vector<string>& handles, bool delete_link_targets) {
    uint count = 0;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("delete_nodes(" << handles.size() << ") from peer [" << uid << "]");
        count = peer->delete_nodes(handles, delete_link_targets);
    }
    return count;
}

uint RemoteAtomDB::delete_links(const vector<string>& handles, bool delete_link_targets) {
    uint count = 0;
    for (auto& [uid, peer] : remote_db_) {
        LOG_DEBUG("delete_links(" << handles.size() << ") from peer [" << uid << "]");
        count = peer->delete_links(handles, delete_link_targets);
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
