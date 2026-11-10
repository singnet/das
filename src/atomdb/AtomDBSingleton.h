#pragma once

#include <memory>

#include "AtomDB.h"
#include "JsonConfig.h"

using namespace std;

namespace atomdb {

class AtomDBSingleton {
   public:
    ~AtomDBSingleton() {}
    static shared_ptr<AtomDB> get_instance();

    /**
     * @brief Installs the AtomDB instance.
     *
     * Fails if the singleton is already initialized.
     */
    static void provide(shared_ptr<AtomDB> atom_db, bool unit_test = false);

   private:
    AtomDBSingleton() {}
    static bool initialized;
    static shared_ptr<AtomDB> atom_db;
};

}  // namespace atomdb
