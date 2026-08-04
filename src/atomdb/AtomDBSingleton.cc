#include "AtomDBSingleton.h"

#include "AdapterDB.h"
#include "AtomDBFactory.h"
#include "RemoteAtomDB.h"
#include "Utils.h"

using namespace atomdb;
using namespace commons;

bool AtomDBSingleton::initialized = false;
shared_ptr<AtomDB> AtomDBSingleton::atom_db = shared_ptr<AtomDB>{};

// --------------------------------------------------------------------------------
// Public methods

void AtomDBSingleton::init(const JsonConfig& atomdb_config) {
    if (AtomDBSingleton::initialized) {
        RAISE_ERROR(
            "AtomDBSingleton already initialized. AtomDBSingleton::init() should be called only once.");
    }

    shared_ptr<AtomDB> atomdb;
    auto atomdb_type = atomdb_config.at_path("type").get_or<string>("");

    if (atomdb_type == "remotedb") {
        auto remote_peers_config =
            atomdb_config.at_path("remote_peers").get_or<JsonConfig>(JsonConfig());
        atomdb = shared_ptr<AtomDB>(new RemoteAtomDB(remote_peers_config));
        atomdb = AtomDBFactory::wrap_if_protected(atomdb);
    } else if (atomdb_type == "adapterdb") {
        atomdb = shared_ptr<AtomDB>(new AdapterDB(atomdb_config));
        atomdb = AtomDBFactory::wrap_if_protected(atomdb);
    } else {
        atomdb = AtomDBFactory::create(atomdb_config);
    }

    AtomDBSingleton::atom_db = atomdb;
    AtomDBSingleton::initialized = true;
}

shared_ptr<AtomDB> AtomDBSingleton::get_instance() {
    if (!AtomDBSingleton::initialized) {
        RAISE_ERROR(
            "Uninitialized AtomDBSingleton. AtomDBSingleton::init() must be called before "
            "AtomDBSingleton::get_instance()");
        return shared_ptr<AtomDB>{};  // To avoid warnings
    } else {
        return AtomDBSingleton::atom_db;
    }
}

void AtomDBSingleton::provide(shared_ptr<AtomDB> atom_db) {
    AtomDBSingleton::atom_db = atom_db;
    AtomDBSingleton::initialized = true;
}
