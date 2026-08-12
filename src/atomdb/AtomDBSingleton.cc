#include "AtomDBSingleton.h"

#include "AtomDBFactory.h"
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
    AtomDBSingleton::atom_db = AtomDBFactory::create(atomdb_config);
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
