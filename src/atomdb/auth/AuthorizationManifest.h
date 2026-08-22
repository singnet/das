#pragma once

#include <map>
#include <string>
#include <vector>

#include "AtomDBAPITypes.h"

using namespace std;

namespace atomdb {

/**
 * @brief In-memory representation of the authorization state.
 *
 * Stores the authorization documents used by the authorization checks.
 * The manifest is independent of the underlying persistence mechanism.
 */
class AuthorizationManifest {
   public:
    AuthorizationManifest() = default;
    ~AuthorizationManifest() = default;

    /**
     * @brief Replaces or inserts the authorization document.
     */
    void set(const atomdb_api_types::AccessPermissionDocument& document);

    /**
     * @brief Adds an authorization entry for public_key.
     *
     * Creates the authorization document if public_key is not registered.
     */
    void add(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry);

    /**
     * @brief Removes an authorization entry from public_key.
     *
     * Does nothing if public_key or the specified entry is not present.
     */
    void remove(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry);

    /**
     * @brief Removes all authorization entries for public_key.
     *
     * Does nothing if public_key is not registered.
     */
    void remove_all(const string& public_key);

    /**
     * @brief Returns whether public_key has an authorization document.
     */
    bool is_registered(const string& public_key) const;

    /**
     * @brief Returns whether public_key has full access.
     */
    bool full_access(const string& public_key);

    /**
     * @brief Returns a document from public_key.
     */
    atomdb_api_types::AccessPermissionDocument* get_document(const string& public_key);

   private:
    map<string, atomdb_api_types::AccessPermissionDocument> documents;
};

}  // namespace atomdb
