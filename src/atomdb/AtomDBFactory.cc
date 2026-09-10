#include "AtomDBFactory.h"

#include "AdapterDB.h"
#include "InMemoryDB.h"
#include "MorkDB.h"
#include "ProtectedAtomDB.h"
#include "RedisMongoDB.h"
#include "RemoteAtomDB.h"
#include "Utils.h"

using namespace atomdb;
using namespace commons;

// --------------------------------------------------------------------------------
// Public methods

shared_ptr<AtomDB> AtomDBFactory::create(const JsonConfig& config) {
    if (config.at_path("uid").is_null()) {
        RAISE_ERROR("AtomDBFactory: missing uid");
    }

    auto atomdb_type = config.at_path("type").get_or<string>("");

    AtomDBType type = AtomDB::string_to_type(atomdb_type);

    shared_ptr<AtomDB> atomdb;

    if (type == AtomDBType::RedisMongoDB || type == AtomDBType::MorkDB ||
        type == AtomDBType::InMemoryDB) {
        atomdb = create_basic_atomdb(config);
    } else if (type == AtomDBType::RemoteAtomDB || type == AtomDBType::AdapterDB) {
        atomdb = create_composite_atomdb(config);
    } else {
        RAISE_ERROR("AtomDBFactory: unsupported AtomDB type: " + atomdb_type);
    }

    return wrap_if_protected(atomdb);
}

// --------------------------------------------------------------------------------
// Private methods

shared_ptr<AtomDB> AtomDBFactory::create_basic_atomdb(const JsonConfig& config) {
    auto atomdb_type = config.at_path("type").get_or<string>("");

    AtomDBType type = AtomDB::string_to_type(atomdb_type);

    shared_ptr<AtomDB> atomdb;

    if (type == AtomDBType::RedisMongoDB) {
        // make_shared cannot access RedisMongoDB's private ctor; friend can via new.
        atomdb = shared_ptr<RedisMongoDB>(new RedisMongoDB(config));
    } else if (type == AtomDBType::MorkDB) {
        atomdb = make_shared<MorkDB>(config);
    } else if (type == AtomDBType::InMemoryDB) {
        atomdb = make_shared<InMemoryDB>(config);
    } else {
        RAISE_ERROR("AtomDBFactory: '" + atomdb_type + "' is not a basic AtomDB type");
    }

    return atomdb;
}

shared_ptr<AtomDB> AtomDBFactory::create_composite_atomdb(const JsonConfig& config) {
    auto atomdb_type = config.at_path("type").get_or<string>("");

    AtomDBType type = AtomDB::string_to_type(atomdb_type);

    shared_ptr<AtomDB> atomdb;

    if (type == AtomDBType::RemoteAtomDB) {
        auto remote_peers_config = config.at_path("remote_peers").get_or<JsonConfig>(JsonConfig());

        map<string, shared_ptr<RemoteAtomDBPeer>> remote_peers;

        for (auto& entry : remote_peers_config) {
            auto peer_config = JsonConfig(entry);
            if (peer_config.at_path("uid").is_null()) {
                RAISE_ERROR("AtomDBFactory: remote peer is missing uid");
            }
            string uid = peer_config.at_path("uid").get_or<string>("");
            if (remote_peers.find(uid) != remote_peers.end()) {
                RAISE_ERROR("AtomDBFactory: duplicate remote peer uid: " + uid);
            }

            shared_ptr<AtomDB> local_persistence = nullptr;
            auto local_persistence_config =
                peer_config.at_path("local_persistence").get_or<JsonConfig>(JsonConfig());
            if (!local_persistence_config.empty()) {
                if (local_persistence_config.at_path("uid").is_null()) {
                    local_persistence_config["uid"] = uid;
                }
                local_persistence = create_basic_atomdb(local_persistence_config);
            }
            remote_peers[uid] =
                make_shared<RemoteAtomDBPeer>(uid, create_basic_atomdb(peer_config), local_persistence);
        }

        atomdb = make_shared<RemoteAtomDB>(config.at_path("uid").get_or<string>(""), remote_peers);
    } else if (type == AtomDBType::AdapterDB) {
        // The backend AtomDB in AdapterDB could be RemoteAtomDB ?
        auto atomdb_backend_config =
            config.at_path("adapterdb.atomdb_backend").get_or<JsonConfig>(JsonConfig());
        auto basic_atomdb = create_basic_atomdb(atomdb_backend_config);
        atomdb = make_shared<AdapterDB>(config, basic_atomdb);
    } else {
        RAISE_ERROR("AtomDBFactory: '" + atomdb_type + "' is not a composite AtomDB type");
    }

    return atomdb;
}

shared_ptr<AtomDB> AtomDBFactory::wrap_if_protected(shared_ptr<AtomDB> atomdb) {
    if (!atomdb) {
        RAISE_ERROR("AtomDBFactory::wrap_if_protected() received null atomdb");
    }

    const auto mode = atomdb->get_protection_mode();

    // Interim: ProtectedAtomDB wrapping is disabled; pass UNPROTECTED backends through unchanged.
    if (mode == atomdb_api_types::ProtectionMode::UNPROTECTED ||
        dynamic_pointer_cast<ProtectedAtomDB>(atomdb)) {
        return atomdb;
    }

    // Interim fail-closed: PROTECTED and FORWARD both require ProtectedAtomDB, which is not
    // enabled yet. RemoteAtomDB may report FORWARD when peers are protected;
    // that case is rejected here until wrapping is restored.
    // TODO: return make_shared<ProtectedAtomDB>(atomdb) when authorization integration is complete.
    if (mode == atomdb_api_types::ProtectionMode::PROTECTED ||
        mode == atomdb_api_types::ProtectionMode::FORWARD) {
        RAISE_ERROR("Protected AtomDB support is not available");
    }

    RAISE_ERROR("AtomDBFactory::wrap_if_protected() encountered unknown protection mode");
}