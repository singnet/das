#include "Keychain.h"

using namespace std;
using namespace atomdb;

// -------------------------------------------------------------------------------------------------
// Constructor

Keychain::Keychain(map<Keychain::AtomDB_UID, Keychain::PublicKey> keys) : keys_(std::move(keys)) {}

// -------------------------------------------------------------------------------------------------
// Public methods

Keychain::PublicKey Keychain::get_public_key(const Keychain::AtomDB_UID& uid) const {
    auto it = this->keys_.find(uid);
    if (it != this->keys_.end()) {
        return it->second;
    }
    return "";
}
