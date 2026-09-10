#pragma once

#include <memory>

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
     * @brief Creates an AtomDB from config and wraps it with ProtectedAtomDB when applicable.
     *
     * @param config AtomDB configuration. Required keys: `"type"` (`redismongodb`, `morkdb`,
     *        `inmemorydb`, `remotedb`, or `adapterdb`) and `"uid"` (may be empty). Each remotedb
     *        peer also requires a `"uid"` key (may be empty; must be unique among peers).
     *
     * Breaking change: `create()` no longer takes a `context` argument. Redis/Mongo namespace
     * isolation is now per backend via optional `JsonConfig["prefix"]` on that backend's
     * config (`redismongodb` / `morkdb`, including each remotedb peer and
     * `adapterdb.atomdb_backend`). The prefix is prepended to Redis keys and MongoDB database
     * and collection names (e.g. `"test_"` yields DB `test_das`). Omit it for the default names.
     */
    static shared_ptr<AtomDB> create(const JsonConfig& config);

   private:
    // Supported types: redismongodb, morkdb, inmemorydb.
    static shared_ptr<AtomDB> create_basic_atomdb(const JsonConfig& config);

    // Supported types: remotedb, adapterdb.
    static shared_ptr<AtomDB> create_composite_atomdb(const JsonConfig& config);

    /**
     * @brief Applies protection wrapping when enabled.
     *
     */
    static shared_ptr<AtomDB> wrap_if_protected(shared_ptr<AtomDB> atomdb);
};

}  // namespace atomdb
