#pragma once

#include <map>
#include <memory>
#include <mutex>
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
 * @brief In-memory representation of the authorization state.
 *
 * Stores the authorization profiles used by the authorization checks.
 */
class AuthorizationManifest {
   public:
    explicit AuthorizationManifest(shared_ptr<AtomDB> atomdb);
    ~AuthorizationManifest() = default;

    /**
     * @brief Checks whether public_key is authorized to perform an operation on an atom.
     */
    bool is_granted(const string& public_key, shared_ptr<Atom> atom, AuthorizationOperation operation);

    /**
     * @brief Checks whether public_key is authorized to perform an operation on a handle.
     */
    bool is_granted(const string& public_key, const string& handle, AuthorizationOperation operation);

    /**
     * @brief Loads public_key's profile into memory if it is not already cached.
     *
     * @return true if a matching access-permission document exists (cached or newly loaded).
     */
    bool ensure_profile_loaded(const string& public_key);

    /**
     * @brief Adds the atoms granted to the keychain's public key to the three counters.
     */
    void count_matching_atoms(size_t& node_count,
                              size_t& link_count,
                              size_t& atom_count,
                              shared_ptr<Keychain> keychain);

   private:
    shared_ptr<AtomDB> atomdb;
    map<string, shared_ptr<AuthorizationProfile>> profiles;
    mutex profiles_mutex;

    /**
     * @brief Shared core behind both public is_granted overloads (atom vs handle).
     *
     * Looks up the profile and resolving the atom (if needed) and running the check.
     */
    bool is_granted(const string& public_key,
                    AuthorizationOperation operation,
                    shared_ptr<Atom> atom,
                    const string& handle);
};

}  // namespace atomdb
