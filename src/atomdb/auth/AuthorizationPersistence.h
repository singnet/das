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
     * @brief Persists authorization schemas for public_key.
     *
     * Each pair is a LinkSchema and a permission bitmask: 1 = read, 2 = write, 3 = read+write.
     */
    virtual void authorize(const string& public_key,
                           vector<pair<LinkSchema, unsigned int>>& schemas) = 0;

    /**
     * @brief Removes all authorization schemas from public_key.
     */
    virtual void revoke(const string& public_key) = 0;
};

}  // namespace atomdb
