#include "AtomDBSingleton.h"

#include "Utils.h"

using namespace atomdb;
using namespace commons;

bool AtomDBSingleton::initialized = false;
shared_ptr<AtomDB> AtomDBSingleton::atom_db = shared_ptr<AtomDB>{};

// --------------------------------------------------------------------------------
// Public methods

shared_ptr<AtomDB> AtomDBSingleton::get_instance() {
    if (!AtomDBSingleton::initialized) {
        RAISE_ERROR(
            "Uninitialized AtomDBSingleton. AtomDBInitializer::init() must be called before "
            "AtomDBSingleton::get_instance()");
        return shared_ptr<AtomDB>{};  // To avoid warnings
    } else {
        return AtomDBSingleton::atom_db;
    }
}

void AtomDBSingleton::provide(shared_ptr<AtomDB> atom_db) {
    if (AtomDBSingleton::initialized) {
        RAISE_ERROR(
            "AtomDBSingleton already initialized. AtomDBInitializer::init() should be called only "
            "once.");
    }
    AtomDBSingleton::atom_db = atom_db;
    AtomDBSingleton::initialized = true;
}
