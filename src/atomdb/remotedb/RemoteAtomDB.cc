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

map<string, string> env_overrides_from_config(const json& config) {
    map<string, string> overrides;
    if (!config.contains("env") || !config["env"].is_object()) return overrides;
    for (auto it = config["env"].begin(); it != config["env"].end(); ++it) {
        string key = it.key();
        if (it.value().is_string()) {
            overrides[key] = it.value().get<string>();
        }
    }
    return overrides;
}

shared_ptr<AtomDB> create_atomdb_from_config(const json& config) {
    string type = config.at("type").get<string>();
    string context = config.value("context", "");

    if (type == "inmemorydb") {
        return make_shared<InMemoryDB>(context.empty() ? "remotedb_" : context);
    }

    EnvironmentRestoreGuard env_guard;
    if (type == "redismongodb") {
        env_guard.save_and_apply(env_overrides_from_config(config));
        RedisMongoDB::initialize_statics(context);
        auto atomdb = make_shared<RedisMongoDB>(context);
        env_guard.restore();
        return atomdb;
    }
    if (type == "morkdb") {
        env_guard.save_and_apply(env_overrides_from_config(config));
        auto atomdb = make_shared<MorkDB>(context);
        env_guard.restore();
        return atomdb;
    }

    Utils::error("Unknown AtomDB type in config: " + type);
    return nullptr;
}

}  // namespace

RemoteAtomDB::RemoteAtomDB(const string& json_file_path) {
    std::ifstream f(json_file_path);
    if (!f.good()) {
        Utils::error("RemoteAtomDB: Cannot open config file: " + json_file_path);
    }
    std::stringstream buf;
    buf << f.rdbuf();
    string json_str = buf.str();

    std::optional<json> doc_opt;
    try {
        doc_opt = json::parse(json_str);
    } catch (const std::exception& e) {
        Utils::error("RemoteAtomDB: Invalid JSON in config: " + string(e.what()));
    }

    auto doc = doc_opt->at("atomdbs");
    for (auto&& peer_config : doc) {
        string uid = peer_config.at("uid");

        shared_ptr<AtomDB> remote = create_atomdb_from_config(peer_config);

        shared_ptr<AtomDB> local_persistence = nullptr;
        if (peer_config.contains("local_persistence")) {
            auto local_it = peer_config.at("local_persistence");
            if (local_it.is_object()) {
                local_persistence = create_atomdb_from_config(local_it);
            }
        }
        if (local_persistence == nullptr) {
            local_persistence = make_shared<InMemoryDB>(uid + "_local");
        }

        remote_db_[uid] = make_shared<RemoteAtomDBPeer>(remote, local_persistence, uid);
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
