#pragma once

#include <map>
#include <string>
#include <vector>

#include "AtomDBAPITypes.h"

using namespace std;

namespace atomdb {

/**
 * @brief In-RAM image of the whole access_permissions collection.
 *
 * AuthorizationManagement keeps exactly one of these. Each AccessPermissionDocument maps 1:1 to one
 * MongoDB document.
 */
class AuthorizationManifest {
   public:
    AuthorizationManifest() = default;

    /** @brief Replaces (or inserts) the document for document.access_key. */
    void set(const atomdb_api_types::AccessPermissionDocument& document);

    /** @brief Adds one entry to public_key, creating the document if needed. */
    void add(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry);

    /** @brief Removes the entry whose schema.handle() == handle from public_key. No-op if absent. */
    void remove(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry);

    /** @brief Removes the whole document for public_key. No-op if not registered. */
    void remove_all(const string& public_key);

    /** @brief Returns true if public_key is registered. */
    bool is_registered(const string& public_key) const;

    /** @brief Returns true if public_key is registered with full_access. */
    bool full_access(const string& public_key);

    atomdb_api_types::AccessPermissionDocument* get_document(const string& public_key);

   private:
    map<string, atomdb_api_types::AccessPermissionDocument> documents;
};

}  // namespace atomdb
