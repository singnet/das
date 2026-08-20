#pragma once

#include <string>

#include "AtomDBAPITypes.h"

using namespace std;

namespace atomdb {

/**
 * @brief Persistence interface for authorization data.
 */
class AuthorizationPersistence {
   public:
    virtual ~AuthorizationPersistence() = default;

    /**
     * @brief Lists all authorization entries for public_key.
     */
    virtual vector<atomdb_api_types::AccessPermissionEntry> list(const string& public_key) = 0;

    /**
     * @brief Persists an authorization entry for public_key.
     */
    virtual void save(const string& public_key,
                      const atomdb_api_types::AccessPermissionEntry& entry) = 0;

    /**
     * @brief Removes an authorization entry from public_key.
     */
    virtual void remove(const string& public_key,
                        const atomdb_api_types::AccessPermissionEntry& entry) = 0;

    /**
     * @brief Removes all authorization entries for public_key.
     */
    virtual void remove_all(const string& public_key) = 0;
};
}  // namespace atomdb
