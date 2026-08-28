#pragma once

#include <memory>
#include <string>

#include "AtomDB.h"
#include "JsonConfig.h"

using namespace std;
using namespace commons;

namespace atomdb {

/**
 * @brief Factory that builds AtomDB instances from a JsonConfig.
 *
 * This is the preferred way to obtain an AtomDB. Callers should not construct
 * RedisMongoDB, MorkDB, InMemoryDB, RemoteAtomDB, or AdapterDB directly; instead
 * pass a config whose "type" field selects the concrete implementation.
 *
 * Two kinds of AtomDB are supported:
 * - Basic: RedisMongoDB, MorkDB, InMemoryDB — constructed from their own config.
 * - Composite: RemoteAtomDB and AdapterDB — built by composing one or more basic
 *   AtomDBs (remote peers for RemoteAtomDB; a wrapped AtomDB for AdapterDB).
 *
 */
class AtomDBFactory {
   public:
    /**
     * @brief Creates an AtomDB. Basic PROTECTED backends are wrapped with ProtectedAtomDB.
     * Composite types (RemoteAtomDB, AdapterDB) are returned as-is; RemoteAtomDB implements
     * AtomDBPublicKeyAPI when any peer is protected.
     */
    static shared_ptr<AtomDB> create(const JsonConfig& config, const string& context = "");

   private:
    // Supported types: redismongodb, morkdb, inmemorydb.
    static shared_ptr<AtomDB> create_basic_atomdb(const JsonConfig& config, const string& context = "");

    // Supported types: remotedb, adapterdb.
    static shared_ptr<AtomDB> create_composite_atomdb(const JsonConfig& config,
                                                      const string& context = "");

    /**
     * @brief Wraps a basic AtomDB with ProtectedAtomDB when its mode is PROTECTED.
     * Types that already implement AtomDBPublicKeyAPI (or are UNPROTECTED) are returned as-is.
     */
    static shared_ptr<AtomDB> wrap_if_protected(shared_ptr<AtomDB> atomdb);
};

}  // namespace atomdb
