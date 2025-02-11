#ifndef _QUERY_ENGINE_ATOMDBSINGLETON_H
#define _QUERY_ENGINE_ATOMDBSINGLETON_H

#include <memory>
#include "AtomDB.h"

using namespace std;

namespace query_engine {

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
    static void init();
    static shared_ptr<AtomDB> get_instance();

private:

    AtomDBSingleton() {}
    static bool initialized;
    static shared_ptr<AtomDB> atom_db;
};

} // namespace query_engine

#endif // _QUERY_ENGINE_ATOMDBSINGLETON_H
