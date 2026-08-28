
#pragma once

#include <memory>
#include <string>

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

    ~AuthorizationManager() = default;

    /**
     * @brief Lists all permissions granted to public_key.
     */
    vector<AuthorizationSchema> list(const string& public_key);

    /**
     * @brief Grants an authorization schema to public_key.
     */
    void authorize(const string& public_key, const AuthorizationSchema& schema);

    /**
     * @brief Revokes an authorization schema from public_key.
     */
    void revoke(const string& public_key, const AuthorizationSchema& schema);

    /**
     * @brief Revokes all authorization schemas from public_key.
     */
    void revoke_all(const string& public_key);

   private:
    shared_ptr<AuthorizationPersistence> persistence;
};

}  // namespace atomdb
