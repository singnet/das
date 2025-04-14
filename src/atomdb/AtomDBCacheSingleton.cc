#include "AtomDBCacheSingleton.h"

#include "Utils.h"

using namespace atomdb;
bool AtomDBCacheSingleton::initialized = false;
shared_ptr<AtomDBCache> AtomDBCacheSingleton::atom_db_cache = shared_ptr<AtomDBCache>{};

void AtomDBCacheSingleton::init() {
    if (AtomDBCacheSingleton::initialized) {
        Utils::error(
            "AtomDBCacheSingleton already initialized. "
            "AtomDBCacheSingleton::init() should be called only once.");
    } else {
        AtomDBCacheSingleton::atom_db_cache = shared_ptr<AtomDBCache>(new AtomDBCache());
        AtomDBCacheSingleton::initialized = true;
    }
}

shared_ptr<AtomDBCache> AtomDBCacheSingleton::get_instance() {
    if (!AtomDBCacheSingleton::initialized) {
        Utils::error(
            "AtomDBCacheSingleton not initialized. "
            "AtomDBCacheSingleton::init() should be called only once.");
    }
    return AtomDBCacheSingleton::atom_db_cache;
}
