#pragma once

#include <memory>
#include <string>
#include <vector>

#include "AuthorizationTypes.h"

using namespace std;

namespace atomdb {

/**
 * @brief Persistence interface for authorization grants.
 *
 * Implementations store and remove authorization state for public keys.
 * Schema-based grants are expressed as LinkSchema patterns;
 * unrestricted grants bypass schema evaluation entirely.
 */
class AuthorizationPersistence {
   public:
    virtual ~AuthorizationPersistence() = default;

    /**
     * @brief Grants schema-based permissions to a public key.
     *
     * Each pair contains a LinkSchema and a permission bitmask:
     * 1 = read, 2 = write, 3 = read + write.
     *
     * Implementations may merge the new schemas with any existing document for
     * the same public key. Schemas with identical token sequences replace the
     * previous entry.
     *
     * @param public_key Public key receiving the grant.
     * @param schemas Schema rules and permission bitmasks to persist.
     * @throws std::runtime_error if the public key already has unrestricted access, or if the underlying
     * store fails to persist the change.
     */
    virtual void grant(const string& public_key, vector<pair<LinkSchema, unsigned int>>& schemas) = 0;

    /**
     * @brief Grants unrestricted access to a public key.
     *
     * Replaces any existing authorization document for the key with a profile
     * that allows all operations on all atoms.
     *
     * @param public_key Public key receiving unrestricted access.
     * @throws std::runtime_error if the underlying store fails to persist the change.
     */
    virtual void grant_unrestricted(const string& public_key) = 0;

    /**
     * @brief Revokes all authorization for a public key.
     *
     * Removes the persisted authorization document for the key, if one exists.
     *
     * @param public_key Public key whose grants should be removed.
     * @throws std::runtime_error if the underlying store fails to remove the document.
     */
    virtual void revoke(const string& public_key) = 0;
};

}  // namespace atomdb
