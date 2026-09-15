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
    explicit Keychain(const map<string, string>& keys);
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
    string get_public_key(const string& uid) const;

   private:
    map<string, string> keys_;
};

}  // namespace atomdb
