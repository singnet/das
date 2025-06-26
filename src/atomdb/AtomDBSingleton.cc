#include "AtomDBSingleton.h"

#include "AtomDBCacheSingleton.h"
#include "Utils.h"

using namespace atomdb;

bool AtomDBSingleton::initialized = false;
shared_ptr<AtomDB> AtomDBSingleton::atom_db = shared_ptr<AtomDB>{};

// --------------------------------------------------------------------------------
// Public methods

void AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE atomdb_type) {
    if (AtomDBSingleton::initialized) {
        Utils::error(
            "AtomDBSingleton already initialized. AtomDBSingleton::init() should be called only once.");
    } else {
        AtomDBCacheSingleton::init();

        if (atomdb_type == atomdb_api_types::ATOMDB_TYPE::MORK_MONGODB) {
            MorkMongoDB::initialize_statics();
            AtomDBSingleton::atom_db = shared_ptr<AtomDB>(new MorkMongoDB());
        } else {
            RedisMongoDB::initialize_statics();
            AtomDBSingleton::atom_db = shared_ptr<AtomDB>(new RedisMongoDB());
        }

        AtomDBSingleton::initialized = true;
    }
}

shared_ptr<AtomDB> AtomDBSingleton::get_instance() {
    if (!AtomDBSingleton::initialized) {
        Utils::error(
            "Uninitialized AtomDBSingleton. AtomDBSingleton::init() must be called before "
            "AtomDBSingleton::get_instance()");
        return shared_ptr<AtomDB>{};  // To avoid warnings
    } else {
        return AtomDBSingleton::atom_db;
    }
}
