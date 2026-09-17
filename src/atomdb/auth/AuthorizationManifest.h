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
     * @brief Returns whether public_key has an authorization document.
     */
    inline bool is_registered(const string& public_key) const {
        lock_guard<mutex> lock(this->profiles_mutex);
        return this->profiles.find(public_key) != this->profiles.end();
    }

    /**
     * @brief Adds an authorization document to the manifest.
     *
     * If access_key is already registered, this is a no-op. Otherwise builds
     * an AuthorizationProfile from the document and stores it keyed by
     * access_key. The registered-check and the insertion are atomic.
     */
    void add_document(const shared_ptr<atomdb_api_types::AccessPermissionDocument>& document);

   private:
    shared_ptr<AtomDB> atomdb;
    map<string, shared_ptr<AuthorizationProfile>> profiles;
    mutable mutex profiles_mutex;
};

}  // namespace atomdb
