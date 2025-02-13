#include "AtomDBSingleton.h"
#include "Utils.h"

using namespace query_engine;
using namespace commons;

bool AtomDBSingleton::initialized = false;
shared_ptr<AtomDB> AtomDBSingleton::atom_db = shared_ptr<AtomDB>{};

// --------------------------------------------------------------------------------
// Public methods

void AtomDBSingleton::init() {
    if (initialized) {
        Utils::error("AtomDBSingleton already initialized. AtomDBSingleton::init() should be called only once.");
    } else {
        AtomDB::initialize_statics();
        atom_db = shared_ptr<AtomDB>(new AtomDB());
        initialized = true;
    }
}

shared_ptr<AtomDB> AtomDBSingleton::get_instance() {
    if (! initialized) {
        Utils::error("Uninitialized AtomDBSingleton. AtomDBSingleton::init() must be called before AtomDBSingleton::get_instance()");
        return shared_ptr<AtomDB>{}; // To avoid warnings
    } else {
        return atom_db;
    }
}
