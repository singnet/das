#pragma once

#include <memory>

#include "AtomDBCache.h"

using namespace std;

namespace atomdb {

/**
 * @brief A singleton class over AtomDBCache.
 */
class AtomDBCacheSingleton {
   public:
    ~AtomDBCacheSingleton() {}

    /**
     * @brief Initializes the AtomDBCache.
     *
     * This method should be called before calling get_instance().
     */
    static void init();

    /**
     * @brief Get the instance of AtomDBCache.
     *
     * @return The instance of AtomDBCache.
     */
    static shared_ptr<AtomDBCache> get_instance();

    static void reset();

   private:
    AtomDBCacheSingleton() {}

    /**
     * @brief True if the AtomDBCache object is created.
     */
    static bool initialized;

    /**
     * @brief The AtomDBCache object.
     */
    static shared_ptr<AtomDBCache> atom_db_cache;
};

}  // namespace atomdb