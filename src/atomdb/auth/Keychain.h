#pragma once

#include <map>
#include <string>

using namespace std;

namespace atomdb {

/**
 * @brief Caller credentials: one public key per AtomDB UID.
 *
 * Holds the map `uid -> public_key` that identifies the caller to each
 * AtomDB. `get_public_key(uid)` returns the key for that database, or
 * an empty string if the UID is absent or stored as empty.
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
