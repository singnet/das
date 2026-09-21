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

   private:
    shared_ptr<AtomDB> atomdb;
    map<string, shared_ptr<AuthorizationProfile>> profiles;
    mutex profiles_mutex;
};

}  // namespace atomdb
