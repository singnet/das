#pragma once

#include <memory>

#include "AtomDB.h"
#include "JsonConfig.h"

using namespace std;

namespace atomdb {

// -------------------------------------------------------------------------------------------------
// NOTE TO REVIEWER:
//
// This class will be replaced/integrated by/with classes already implemented in das-atom-db.
//
// I think it's pointless to make any further documentation while we don't make this integration.
// -------------------------------------------------------------------------------------------------

class AtomDBSingleton {
   public:
    ~AtomDBSingleton() {}
    static shared_ptr<AtomDB> get_instance();

    /**
     * @brief Installs the AtomDB instance.
     *
     * Fails if the singleton is already initialized. Replacing the current instance is an
     * explicit decision: call reset() and then provide() again.
     */
    static void provide(shared_ptr<AtomDB> atom_db);

    /**
     * @brief Drops the current AtomDB instance.
     *
     * After reset(), get_instance() fails until provide() installs another instance.
     * Swapping the AtomDB is reset() followed by provide().
     */
    static void reset();

   private:
    AtomDBSingleton() {}
    static bool initialized;
    static shared_ptr<AtomDB> atom_db;
};

}  // namespace atomdb
