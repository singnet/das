#include "AtomDBInitializer.h"

#include "AtomDBFactory.h"
#include "AtomDBSingleton.h"

using namespace atomdb;
using namespace commons;

// --------------------------------------------------------------------------------
// Public methods

void AtomDBInitializer::init(const commons::JsonConfig& atomdb_config) {
    auto atom_db = AtomDBFactory::create(atomdb_config);
    AtomDBSingleton::provide(atom_db);
}
