#include "AtomDBSingleton.h"

#include "AdapterDB.h"
#include "MorkDB.h"
#include "RedisMongoDB.h"
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
    } else {
        auto atomdb_type = atomdb_config.at_path("type").get_or<string>("");
        if (atomdb_type == "morkdb") {
            AtomDBSingleton::atom_db = shared_ptr<AtomDB>(new MorkDB("", atomdb_config));
        } else if (atomdb_type == "redismongodb") {
            AtomDBSingleton::atom_db = shared_ptr<AtomDB>(new RedisMongoDB("", false, atomdb_config));
        } else if (atomdb_type == "remotedb") {
            AtomDBSingleton::atom_db = shared_ptr<AtomDB>(new RemoteAtomDB(atomdb_config));
        } else if (atomdb_type == "adapterdb") {
            AtomDBSingleton::atom_db = shared_ptr<AtomDB>(new AdapterDB(atomdb_config));
        } else {
            RAISE_ERROR("Invalid AtomDB type: " + atomdb_type);
        }

        AtomDBSingleton::initialized = true;
    }
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
