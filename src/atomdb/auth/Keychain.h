#pragma once

#include <map>
#include <string>

using namespace std;

namespace atomdb {

/**
 * @brief Caller credentials: public keys keyed by AtomDB UID.
 *
 * Keychain is the identity token passed into ProtectedAtomDB.
 * AuthorizationManifest looks up the public key for a given
 * database UID and checks the corresponding AuthorizationProfile.
 */
class Keychain {
   public:
    using AtomDB_UID = string;
    using PublicKey = string;

    explicit Keychain(map<AtomDB_UID, PublicKey> keys);
    ~Keychain() = default;

    /**
     * @brief Returns the public key registered for `uid`.
     *
     * Empty string means no usable key: the UID is missing or its stored
     * value is empty.
     *
     * @param uid AtomDB UID to look up.
     * @return The stored public key, or empty string.
     */
    PublicKey get_public_key(const AtomDB_UID& uid) const;

   private:
    map<AtomDB_UID, PublicKey> keys_;
};

}  // namespace atomdb
