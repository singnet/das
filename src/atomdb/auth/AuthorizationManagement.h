#pragma once

#include <memory>
#include <string>

#include "Atom.h"
#include "AtomDBAPITypes.h"
#include "AuthorizationManifest.h"
#include "AuthorizationPersistence.h"
#include "HandleDecoder.h"

using namespace std;
using namespace atoms;

namespace atomdb {

class AtomDB;

enum class AuthorizationOperation { READ, WRITE };

/**
 * @brief Authorization queries and administration.
 *
 * The manifest is kept in RAM; authorize() and revoke*() update both the storage (through
 * AuthorizationPersistence) and the in-RAM manifest.
 */
class AuthorizationManagement {
   public:
    /**
     * @param persistence Storage used by authorize() and revoke*().
     */
    AuthorizationManagement(shared_ptr<AuthorizationPersistence> persistence);

    /**
     * @brief Checks whether public_key may perform operation on atom.
     */
    bool is_authorized(const Atom& atom,
                       const string& public_key,
                       AuthorizationOperation operation,
                       HandleDecoder& decoder);

    /**
     * @brief Checks whether public_key may perform operation on handle.
     */
    bool is_authorized(const string& handle,
                       const string& public_key,
                       AuthorizationOperation operation,
                       HandleDecoder& decoder);

    /**
     * @brief Grants one entry to public_key. Updates storage and the in-RAM manifest.
     */
    void authorize(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry);

    /**
     * @brief Revokes one entry from public_key. Updates storage and the in-RAM manifest.
     */
    void revoke(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry);

    /**
     * @brief Revokes every entry of public_key. Updates storage and the in-RAM manifest.
     */
    void revoke_all(const string& public_key);

   private:
    shared_ptr<AuthorizationPersistence> persistence;
    AuthorizationManifest manifest;

    bool matches_schema(const LinkSchema& schema, const Atom& atom, HandleDecoder& decoder) const;
    bool matches_schema(const LinkSchema& schema, const string& handle, HandleDecoder& decoder) const;

    static bool allows(const atomdb_api_types::AccessPermissionEntry& entry,
                       AuthorizationOperation operation);
};

}  // namespace atomdb
