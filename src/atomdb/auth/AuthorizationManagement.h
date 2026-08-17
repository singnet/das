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
     * @param atomdb AtomDB used as HandleDecoder.
     * @param persistence Storage used by authorize() and revoke*(). May be null when the atomdb
     *        has no authorization storage; administration then fails.
     */
    AuthorizationManagement(shared_ptr<AtomDB> atomdb, shared_ptr<AuthorizationPersistence> persistence);

    /**
     * @brief Returns true when public_key is registered with full_access.
     */
    bool has_full_access(const string& public_key);

    /**
     * @brief Checks whether public_key may perform operation on atom.
     *
     * @return true if full_access or the atom matches at least one entry allowing operation.
     */
    bool is_authorized(const Atom& atom, const string& public_key, AuthorizationOperation operation);

    /**
     * @brief Checks whether public_key may perform operation on handle.
     *
     * @param decoder HandleDecoder from the atomdb (required by LinkSchema::match).
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
     * @brief Revokes one entry from public_key.
     *
     * @param handle AccessPermissionEntry::schema.handle(), i.e. the LinkSchema handle.
     */
    void revoke(const string& public_key, const string& handle);

    /**
     * @brief Revokes every entry of public_key. Updates storage and the in-RAM manifest.
     */
    void revoke_all(const string& public_key);

   private:
    shared_ptr<AtomDB> atomdb;
    shared_ptr<AuthorizationPersistence> persistence;
    AuthorizationManifest manifest;

    bool matches_entry(const atomdb_api_types::AccessPermissionEntry& entry,
                       const Atom& atom,
                       HandleDecoder& decoder) const;
    bool matches_entry(const atomdb_api_types::AccessPermissionEntry& entry,
                       const string& handle,
                       HandleDecoder& decoder) const;

    static bool allows(const atomdb_api_types::AccessPermissionEntry& entry,
                       AuthorizationOperation operation);
};

}  // namespace atomdb
