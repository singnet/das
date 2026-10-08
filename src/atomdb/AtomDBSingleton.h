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
     * Fails if the singleton is already initialized.
     */
    static void provide(shared_ptr<AtomDB> atom_db);

   private:
    AtomDBSingleton() {}
    static bool initialized;
    static shared_ptr<AtomDB> atom_db;
};

}  // namespace atomdb
