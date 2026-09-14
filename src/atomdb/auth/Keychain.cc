#include "Keychain.h"

using namespace std;
using namespace atomdb;

// -------------------------------------------------------------------------------------------------
// Constructor

Keychain::Keychain(const map<string, string>& keys) { this->keys_ = keys; }

// -------------------------------------------------------------------------------------------------
// Public methods

string Keychain::get_public_key(const string& uid) const {
    auto it = this->keys_.find(uid);
    if (it != this->keys_.end()) {
        return it->second;
    }
    return "";
}
