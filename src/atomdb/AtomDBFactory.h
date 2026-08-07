#pragma once

#include <memory>
#include <string>

#include "AtomDB.h"
#include "JsonConfig.h"

using namespace std;
using namespace commons;

namespace atomdb {

/**
 * @brief Single entry point to construct concrete AtomDB backends.
 *
 * Use this instead of calling RedisMongoDB/MorkDB/InMemoryDB constructors directly.
 */
class AtomDBFactory {
   public:
    /**
     * @brief Creates a AtomDB and wraps it with ProtectedAtomDB when is_protected().
     */
    static shared_ptr<AtomDB> create(const JsonConfig& config,
                                     const string& context = "",
                                     bool should_wrap = true);

   private:
    /**
     * @brief Creates a concrete AtomDB without authorization wrapping.
     */
    static shared_ptr<AtomDB> create_atomdb(const JsonConfig& config, const string& context = "");

    // Supported types: redismongodb, morkdb, inmemorydb.
    static shared_ptr<AtomDB> create_basic_atomdb(const JsonConfig& config, const string& context = "");

    // Supported types: remotedb, adapterdb.
    static shared_ptr<AtomDB> create_composite_atomdb(const JsonConfig& config,
                                                      const string& context = "");

    /**
     * @brief Wraps backend with ProtectedAtomDB when protected and not already wrapped.
     */
    static shared_ptr<AtomDB> wrap_if_protected(shared_ptr<AtomDB> backend);
};

}  // namespace atomdb
