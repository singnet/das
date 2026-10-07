#include "AtomDBInitializer.h"

#include "AtomDBFactory.h"
#include "AtomDBSingleton.h"
#include "Utils.h"

using namespace atomdb;
using namespace commons;

// --------------------------------------------------------------------------------
// Public methods

void AtomDBInitializer::init(const JsonConfig& atomdb_config) {
    if (AtomDBSingleton::is_initialized()) {
        RAISE_ERROR(
            "AtomDBSingleton already initialized. AtomDBInitializer::init() should be called only "
            "once.");
    }
    auto atom_db = AtomDBFactory::create(atomdb_config);
    AtomDBSingleton::provide(atom_db);
}
