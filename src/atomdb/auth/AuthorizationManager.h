
#pragma once

#include <memory>
#include <string>

#include "AtomDBAPITypes.h"
#include "AuthorizationPersistence.h"

using namespace std;
using namespace atoms;

namespace atomdb {

/**
 * @brief Manages authorization permissions through persistent storage.
 */
class AuthorizationManager {
   public:
    /**
     * @param persistence Storage used to manage authorization permissions.
     */
    AuthorizationManager(shared_ptr<AuthorizationPersistence> persistence);

    /**
     * @brief Lists all permissions granted to public_key.
     */
    vector<atomdb_api_types::AccessPermissionEntry> list(const string& public_key);

    /**
     * @brief Grants an authorization entry to public_key.
     */
    void authorize(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry);

    /**
     * @brief Revokes an authorization entry from public_key.
     */
    void revoke(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry);

    /**
     * @brief Revokes all authorization entries from public_key.
     */
    void revoke_all(const string& public_key);

   private:
    shared_ptr<AuthorizationPersistence> persistence;
};

}  // namespace atomdb