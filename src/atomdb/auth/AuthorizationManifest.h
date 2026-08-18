#pragma once

#include <map>
#include <string>
#include <vector>

#include "AtomDBAPITypes.h"

using namespace std;

namespace atomdb {

namespace auth {

/**
 * @brief In-RAM image of the whole access_permissions collection.
 *
 * AuthorizationManagement keeps exactly one of these. Each AccessPermissionDocument maps 1:1 to one
 * MongoDB document.
 */
class AuthorizationManifest {
   public:
    AuthorizationManifest() = default;

    /** @brief Replaces (or inserts) the document for document.public_key. */
    void set(const atomdb_api_types::AccessPermissionDocument& document);

    /** @brief Adds one entry to public_key, creating the document if needed. */
    void add(const atomdb_api_types::PublicKey& public_key,
             const atomdb_api_types::AccessPermissionEntry& entry);

    /** @brief Removes the entry whose schema.handle() == handle from public_key. No-op if absent. */
    void remove(const atomdb_api_types::PublicKey& public_key, const string& handle);

    /** @brief Removes the whole document for public_key. No-op if not registered. */
    void remove_all(const atomdb_api_types::PublicKey& public_key);

    /** @brief Returns true if public_key is registered. */
    bool is_registered(const atomdb_api_types::PublicKey& public_key) const;

    /** @brief Returns true if public_key is registered with full_access. */
    bool full_access(const atomdb_api_types::PublicKey& public_key);

    /** @brief Returns the entries for public_key, or an empty vector if not registered. */
    const vector<atomdb_api_types::AccessPermissionEntry>& entries(
        const atomdb_api_types::PublicKey& public_key);

   private:
    map<string, atomdb_api_types::AccessPermissionDocument> documents;
    static const vector<atomdb_api_types::AccessPermissionEntry> EMPTY_ENTRIES;

    void create_document(const atomdb_api_types::PublicKey& public_key,
                         const atomdb_api_types::AccessPermissionEntry& entry);
    atomdb_api_types::AccessPermissionDocument* find_document(
        const atomdb_api_types::PublicKey& public_key, const string& caller);
};

}  // namespace auth
}  // namespace atomdb
