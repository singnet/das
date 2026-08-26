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
 * @brief In-memory representation of the authorization state.
 *
 * Stores the authorization profiles used by the authorization checks.
 */
class AuthorizationManifest {
   public:
    AuthorizationManifest() = default;
    ~AuthorizationManifest() = default;

    /**
     * @brief Checks whether public_key is authorized to perform an operation on an atom.
     */
    bool is_authorized(const Atom& atom,
                       const string& public_key,
                       AuthorizationOperation operation,
                       HandleDecoder& decoder);

    /**
     * @brief Checks whether public_key is authorized to perform an operation on a handle.
     */
    bool is_authorized(const string& handle,
                       const string& public_key,
                       AuthorizationOperation operation,
                       HandleDecoder& decoder);
    /**
     * @brief Returns whether public_key has an authorization document.
     */
    bool is_registered(const string& public_key) const;

    /**
     * @brief Returns whether public_key has full access.
     */
    bool full_access(const string& public_key);

    /**
     * @brief Returns a profile from public_key.
     */
    AuthorizationProfile* lookup(const string& public_key);

    /**
     * @brief Adds an authorization document to the manifest.
     *
     * Builds an AuthorizationProfile from the document and stores it in the
     * in-memory cache keyed by access_key. Raises an error if the access_key
     * is already registered.
     */
    void add_document(const shared_ptr<atomdb_api_types::AccessPermissionDocument>& document);

   private:
    map<string, AuthorizationProfile> profiles;
};

}  // namespace atomdb
