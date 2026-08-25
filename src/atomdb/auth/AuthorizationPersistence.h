#pragma once

#include <memory>
#include <string>
#include <vector>

#include "AuthorizationTypes.h"

using namespace std;

namespace atomdb {

/**
 * @brief Storage interface for authorization permission CRUD operations.
 */
class AuthorizationPersistence {
   public:
    virtual ~AuthorizationPersistence() = default;

    /**
     * @brief Lists all authorization entries for public_key.
     */
    virtual vector<AuthorizationSchema> list(const string& public_key) = 0;

    /**
     * @brief Persists an authorization entry for public_key.
     */
    virtual void save(const string& public_key, const AuthorizationSchema& entry) = 0;

    /**
     * @brief Removes an authorization entry from public_key.
     */
    virtual void remove(const string& public_key, const AuthorizationSchema& entry) = 0;

    /**
     * @brief Removes all authorization entries for public_key.
     */
    virtual void remove_all(const string& public_key) = 0;
};

}  // namespace atomdb
