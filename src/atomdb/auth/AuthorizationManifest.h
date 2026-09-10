#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Atom.h"
#include "AtomDBAPITypes.h"
#include "AuthorizationTypes.h"
#include "HandleDecoder.h"

using namespace std;
using namespace atoms;

namespace atomdb {

/**
 * @brief In-memory authorization state keyed by public key.
 *
 * AuthorizationManifest stores one AuthorizationProfile per registered public key
 * and answers grant checks against atoms or handles. It is the runtime view used
 * by ProtectedAtomDB when enforcing access control.
 */
class AuthorizationManifest {
   public:
    /**
     * @brief Creates an empty manifest with the given AtomDB.
     *
     * @param atomdb AtomDB used to resolve atoms when authorizing by handle.
     */
    explicit AuthorizationManifest(shared_ptr<AtomDB> atomdb);

    ~AuthorizationManifest() = default;

    /**
     * @brief Checks whether a public key is granted an operation on an atom.
     *
     * Returns false when the public key is not registered.
     *
     * @param public_key Public key to authorize.
     * @param atom Target atom.
     * @param operation Operation being requested.
     * @return true if the public key is granted the operation on the atom.
     */
    bool is_granted(const string& public_key, shared_ptr<Atom> atom, AuthorizationOperation operation);

    /**
     * @brief Checks whether a public key is granted an operation on a handle.
     *
     * Resolves the handle through the manifest AtomDB before evaluating the
     * profile. Returns false when the public key is not registered, the handle
     * cannot be resolved, or the atom does not exist.
     *
     * @param public_key Public key to authorize.
     * @param handle Target atom handle.
     * @param operation Operation being requested.
     * @return true if the public key is granted the operation on the handle.
     */
    bool is_granted(const string& public_key, const string& handle, AuthorizationOperation operation);

    /**
     * @brief Returns whether a public key has a registered authorization profile.
     *
     * @param public_key Public key to look up.
     * @return true if a profile exists for the public key.
     */
    inline bool is_registered(const string& public_key) const {
        return this->profiles.find(public_key) != this->profiles.end();
    }

    /**
     * @brief Registers an authorization document in the manifest.
     *
     * Builds an AuthorizationProfile from the document and stores it under
     * document->get_access_key().
     *
     * @param document Authorization document to register.
     * @throws std::runtime_error if document is null or access_key is already registered.
     */
    void add_document(const shared_ptr<atomdb_api_types::AccessPermissionDocument>& document);

   private:
    shared_ptr<AtomDB> atomdb;
    map<string, shared_ptr<AuthorizationProfile>> profiles;
};

}  // namespace atomdb
