#include "RemoteAtomDB.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>

#include "EnvironmentRestoreGuard.h"
#include "InMemoryDB.h"
#include "InMemoryDBAPITypes.h"
#include "MorkDB.h"
#include "RedisMongoDB.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace atoms;
using namespace commons;

using json = nlohmann::json;

namespace {

map<string, string> env_overrides_from_config(const JsonConfig& config) {
    map<string, string> overrides;
    auto env_val = config.at_path("env");
    if (env_val.is_null() || !(*env_val).is_object()) return overrides;
    for (auto& [key, value] : (*env_val).items()) {
        if (value.is_string()) {
            overrides[key] = value.get<string>();
        }
    }
    return overrides;
}

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

    Utils::error("Unknown AtomDB type for peer " + uid + ": " + type);
    return nullptr;
}

}  // namespace

RemoteAtomDB::RemoteAtomDB(const JsonConfig& peers_config) {
    for (auto& entry : peers_config) {
        auto peer_config = JsonConfig(entry);
        string uid = peer_config.at_path("uid").get_or<string>("");
        if (uid.empty()) continue;
        remote_db_[uid] =
            make_shared<RemoteAtomDBPeer>(create_atomdb_from_config(peer_config), nullptr, uid);
    }

    LOG_INFO("RemoteAtomDB initialized with " << remote_db_.size() << " remote peers");
}

RemoteAtomDB::~RemoteAtomDB() = default;

bool RemoteAtomDB::allow_nested_indexing() { return false; }

shared_ptr<Atom> RemoteAtomDB::get_atom(const string& handle) {
    for (auto& [uid, peer] : remote_db_) {
        auto atom = peer->get_atom(handle);
        if (atom) return atom;
    }
    return nullptr;
}

shared_ptr<Node> RemoteAtomDB::get_node(const string& handle) {
    for (auto& [uid, peer] : remote_db_) {
        auto node = peer->get_node(handle);
        if (node) return node;
    }
    return nullptr;
}

shared_ptr<Link> RemoteAtomDB::get_link(const string& handle) {
    for (auto& [uid, peer] : remote_db_) {
        auto link = peer->get_link(handle);
        if (link) return link;
    }
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

    for (auto& [uid, peer] : remote_db_) {
        auto handle_set = peer->query_for_pattern(link_schema);
        if (!handle_set) continue;

        auto it = handle_set->get_iterator();
        if (!it) continue;

        while (true) {
            char* h = it->next();
            if (!h) break;
            string handle(h);
            if (seen.insert(handle).second) {
                result->add_handle(handle);
            }
        }
    }
    return result;
}

shared_ptr<atomdb_api_types::HandleList> RemoteAtomDB::query_for_targets(const string& handle) {
    for (auto& [uid, peer] : remote_db_) {
        auto list = peer->query_for_targets(handle);
        if (list) return list;
    }
    return nullptr;
}

shared_ptr<atomdb_api_types::HandleSet> RemoteAtomDB::query_for_incoming_set(const string& handle) {
    auto result = make_shared<atomdb_api_types::HandleSetInMemory>();
    set<string> seen;

    for (auto& [uid, peer] : remote_db_) {
        auto handle_set = peer->query_for_incoming_set(handle);
        if (!handle_set) continue;

        auto it = handle_set->get_iterator();
        if (!it) continue;

        while (true) {
            char* h = it->next();
            if (!h) break;
            string handle(h);
            if (seen.insert(handle).second) {
                result->add_handle(handle);
            }
        }
    }
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
        handle = peer->add_atom(atom, throw_if_exists);
    }
    return handle;
}

string RemoteAtomDB::add_node(const atoms::Node* node, bool throw_if_exists) {
    string handle;
    for (auto& [uid, peer] : remote_db_) {
        handle = peer->add_node(node, throw_if_exists);
    }
    return handle;
}

string RemoteAtomDB::add_link(const atoms::Link* link, bool throw_if_exists) {
    string handle;
    for (auto& [uid, peer] : remote_db_) {
        handle = peer->add_link(link, throw_if_exists);
    }
    return handle;
}

vector<string> RemoteAtomDB::add_atoms(const vector<atoms::Atom*>& atoms,
                                       bool throw_if_exists,
                                       bool is_transactional) {
    vector<string> handles;
    for (auto& [uid, peer] : remote_db_) {
        handles = peer->add_atoms(atoms, throw_if_exists, is_transactional);
    }
    return handles;
}

vector<string> RemoteAtomDB::add_nodes(const vector<atoms::Node*>& nodes,
                                       bool throw_if_exists,
                                       bool is_transactional) {
    vector<string> handles;
    for (auto& [uid, peer] : remote_db_) {
        handles = peer->add_nodes(nodes, throw_if_exists, is_transactional);
    }
    return handles;
}

vector<string> RemoteAtomDB::add_links(const vector<atoms::Link*>& links,
                                       bool throw_if_exists,
                                       bool is_transactional) {
    vector<string> handles;
    for (auto& [uid, peer] : remote_db_) {
        handles = peer->add_links(links, throw_if_exists, is_transactional);
    }
    return handles;
}

bool RemoteAtomDB::delete_atom(const string& handle, bool delete_link_targets) {
    bool ok = true;
    for (auto& [uid, peer] : remote_db_) {
        ok = peer->delete_atom(handle, delete_link_targets) && ok;
    }
    return ok;
}

bool RemoteAtomDB::delete_node(const string& handle, bool delete_link_targets) {
    bool ok = true;
    for (auto& [uid, peer] : remote_db_) {
        ok = peer->delete_node(handle, delete_link_targets) && ok;
    }
    return ok;
}

bool RemoteAtomDB::delete_link(const string& handle, bool delete_link_targets) {
    bool ok = true;
    for (auto& [uid, peer] : remote_db_) {
        ok = peer->delete_link(handle, delete_link_targets) && ok;
    }
    return ok;
}

uint RemoteAtomDB::delete_atoms(const vector<string>& handles, bool delete_link_targets) {
    uint count = 0;
    for (auto& [uid, peer] : remote_db_) {
        count = peer->delete_atoms(handles, delete_link_targets);
    }
    return count;
}

uint RemoteAtomDB::delete_nodes(const vector<string>& handles, bool delete_link_targets) {
    uint count = 0;
    for (auto& [uid, peer] : remote_db_) {
        count = peer->delete_nodes(handles, delete_link_targets);
    }
    return count;
}

uint RemoteAtomDB::delete_links(const vector<string>& handles, bool delete_link_targets) {
    uint count = 0;
    for (auto& [uid, peer] : remote_db_) {
        count = peer->delete_links(handles, delete_link_targets);
    }
    return count;
}

void RemoteAtomDB::re_index_patterns(bool flush_patterns) {
    for (auto& [uid, peer] : remote_db_) {
        peer->re_index_patterns(flush_patterns);
    }
}

RemoteAtomDBPeer* RemoteAtomDB::get_peer(const string& uid) {
    auto it = remote_db_.find(uid);
    return (it != remote_db_.end()) ? it->second.get() : nullptr;
}
