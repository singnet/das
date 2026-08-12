#pragma once

#include <memory>
#include <string>

#include "Atom.h"
#include "AuthorizationEntry.h"
#include "AuthorizationManifest.h"
#include "AuthorizationPersistence.h"
#include "HandleDecoder.h"

using namespace std;
using namespace atoms;

namespace atomdb {

class AtomDB;

/**
 * @brief Authorization queries and administration.
 *
 * The manifest is loaded once in the constructor and kept in RAM; authorize() and revoke*() update both
 * the storage (through AuthorizationPersistence) and the in-RAM manifest, so no lookup ever hits the
 * database.
 */
class AuthorizationManagement {
   public:
    /**
     * @brief Builds the in-RAM AuthorizationManifest from atomdb->get_access_permissions().
     *
     * @param atomdb AtomDB used to read access_permissions JSON documents and as HandleDecoder.
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
    void authorize(const string& public_key, const AuthorizationEntry& entry);

    /**
     * @brief Revokes one entry from public_key.
     *
     * @param handle AuthorizationEntry::handle(), i.e. the LinkSchema handle.
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

    bool matches_entry(const AuthorizationEntry& entry, const Atom& atom, HandleDecoder& decoder) const;
    bool matches_entry(const AuthorizationEntry& entry,
                       const string& handle,
                       HandleDecoder& decoder) const;

    /** @brief Parses one JSON document from AtomDB::get_access_permissions() into a Document. */
    static AuthorizationManifest::Document parse_access_permissions_document(const string& json);
};

}  // namespace atomdb
