#pragma once

#include <string>

#include "AtomDBAPITypes.h"

using namespace std;

namespace atomdb {

/** @brief Storage interface for authorization writes. */
class AuthorizationPersistence {
   public:
    virtual ~AuthorizationPersistence() = default;

    /** @brief Persists one entry under public_key (creating the document if needed). */
    virtual void save(const atomdb_api_types::PublicKey& public_key,
                      const atomdb_api_types::AccessPermissionDocument& entry) = 0;

    /** @brief Removes the entry identified */
    virtual void remove(const atomdb_api_types::PublicKey& public_key, const atomdb_api_types::AccessPermissionEntry& entry) = 0;

    /** @brief Removes the whole document for public_key. */
    virtual void remove_all(const atomdb_api_types::PublicKey& public_key) = 0;
};

}  // namespace atomdb
