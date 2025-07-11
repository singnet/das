#pragma once

#include <memory>

#include "MorkDB.h"
#include "RedisMongoDB.h"

using namespace std;

namespace atomdb {

// -------------------------------------------------------------------------------------------------
// NOTE TO REVIEWER:
//
// This class will be replaced/integrated by/with classes already implemented in das-atom-db.
//
// I think it's pointless to make any further documentation while we don't make this integrfation.
// -------------------------------------------------------------------------------------------------

class AtomDBSingleton {
   public:
    ~AtomDBSingleton() {}
    static void init(
        atomdb_api_types::ATOMDB_TYPE atomdb_type = atomdb_api_types::ATOMDB_TYPE::REDIS_MONGODB);
    static shared_ptr<AtomDB> get_instance();
    static void provide(shared_ptr<AtomDB> atom_db);

   private:
    AtomDBSingleton() {}
    static bool initialized;
    static shared_ptr<AtomDB> atom_db;
};

}  // namespace atomdb
