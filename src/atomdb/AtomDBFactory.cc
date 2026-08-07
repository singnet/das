#include "AtomDBFactory.h"

#include "AdapterDB.h"
#include "InMemoryDB.h"
#include "MorkDB.h"
#include "RedisMongoDB.h"
#include "RemoteAtomDB.h"
#include "Utils.h"

using namespace atomdb;
using namespace commons;

// --------------------------------------------------------------------------------
// Public methods

shared_ptr<AtomDB> AtomDBFactory::create(const JsonConfig& config,
                                         const string& context,
                                         bool should_wrap) {
    auto atomdb = create_atomdb(config, context);
    if (should_wrap) {
        return wrap_if_protected(atomdb);
    }
    return atomdb;
}

// --------------------------------------------------------------------------------
// Private methods

shared_ptr<AtomDB> AtomDBFactory::create_atomdb(const JsonConfig& config, const string& context) {
    auto atomdb_type = config.at_path("type").get_or<string>("");

    AtomDBType type = parse_atomdb_type(atomdb_type);

    if (type == AtomDBType::RedisMongoDB || type == AtomDBType::MorkDB ||
        type == AtomDBType::InMemoryDB) {
        return create_basic_atomdb(config, context);
    }

    if (type == AtomDBType::RemoteAtomDB || type == AtomDBType::AdapterDB) {
        return create_composite_atomdb(config, context);
    }

    RAISE_ERROR("AtomDBFactory: unsupported AtomDB type: " + atomdb_type);

    return shared_ptr<AtomDB>{};
}

shared_ptr<AtomDB> AtomDBFactory::create_basic_atomdb(const JsonConfig& config, const string& context) {
    auto atomdb_type = config.at_path("type").get_or<string>("");

    AtomDBType type = parse_atomdb_type(atomdb_type);

    if (type == AtomDBType::RedisMongoDB) {
        return shared_ptr<AtomDB>(new RedisMongoDB(context, false, config));
    }

    if (type == AtomDBType::MorkDB) {
        return shared_ptr<AtomDB>(new MorkDB(context, config));
    }

    if (type == AtomDBType::InMemoryDB) {
        return shared_ptr<AtomDB>(new InMemoryDB(context.empty() ? "inmemorydb_" : context));
    }

    RAISE_ERROR("AtomDBFactory: '" + atomdb_type + "' is not a basic AtomDB type");

    return shared_ptr<AtomDB>{};
}

shared_ptr<AtomDB> AtomDBFactory::create_composite_atomdb(const JsonConfig& config,
                                                          const string& context) {
    auto atomdb_type = config.at_path("type").get_or<string>("");

    AtomDBType type = parse_atomdb_type(atomdb_type);

    if (type == AtomDBType::RemoteAtomDB) {
        auto remote_peers_config = config.at_path("remote_peers").get_or<JsonConfig>(JsonConfig());

        map<string, shared_ptr<RemoteAtomDBPeer>> remote_peers;

        for (auto& entry : remote_peers_config) {
            auto peer_config = JsonConfig(entry);
            string uid = peer_config.at_path("uid").get_or<string>("");
            if (uid.empty()) {
                RAISE_ERROR("AtomDBFactory: remote peer is missing a non-empty uid");
            }

            string peer_context = peer_config.at_path("context").get_or<string>("");
            if (peer_context.empty()) {
                peer_context = "remotedb_" + uid;
            }

            shared_ptr<AtomDB> local_persistence = nullptr;
            auto local_persistence_config =
                peer_config.at_path("local_persistence").get_or<JsonConfig>(JsonConfig());
            if (!local_persistence_config.empty()) {
                string local_context =
                    local_persistence_config.at_path("context").get_or<string>(peer_context);
                if (local_context.empty()) {
                    local_context = peer_context;
                }
                local_persistence = create_basic_atomdb(local_persistence_config, local_context);
            }
            remote_peers[uid] = make_shared<RemoteAtomDBPeer>(
                create_basic_atomdb(peer_config, peer_context), local_persistence, uid);
        }

        return shared_ptr<AtomDB>(new RemoteAtomDB(remote_peers));
    }

    if (type == AtomDBType::AdapterDB) {
        // The backend AtomDB in AdapterDB could be RemoteAtomDB ?
        auto atomdb_backend_config =
            config.at_path("adapterdb.atomdb_backend").get_or<JsonConfig>(JsonConfig());
        auto basic_atomdb = create_basic_atomdb(atomdb_backend_config, context);
        return shared_ptr<AtomDB>(new AdapterDB(config, basic_atomdb));
    }

    return shared_ptr<AtomDB>{};
}

shared_ptr<AtomDB> AtomDBFactory::wrap_if_protected(shared_ptr<AtomDB> backend) {
    // AtomDBFactory::wrap_if_protected() is not implemented yet.
    return backend;
}