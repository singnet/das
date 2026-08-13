#include "PublicKey.h"

#include "Utils.h"

using namespace atomdb;
using namespace commons;

PublicKey::PublicKey(const string& key) : single_key(key) {
    if (key.empty()) {
        RAISE_ERROR("PublicKey: key must not be empty");
    }
}

PublicKey::PublicKey(const map<string, string>& keys) : peer_keys(keys) {
    if (keys.empty()) {
        RAISE_ERROR("PublicKey: keys must not be empty");
    }
    for (const auto& [peer_uid, key] : keys) {
        if (peer_uid.empty()) {
            RAISE_ERROR("PublicKey: peer uid must not be empty");
        }
        if (key.empty()) {
            RAISE_ERROR("PublicKey: key for peer '" + peer_uid + "' must not be empty");
        }
    }
}

bool PublicKey::is_single_key() const { return !this->single_key.empty(); }

bool PublicKey::is_peer_map() const { return !this->peer_keys.empty(); }

const string& PublicKey::key() const {
    if (!this->is_single_key()) {
        RAISE_ERROR("PublicKey::key() called on a peer-map instance");
    }
    return this->single_key;
}

const map<string, string>& PublicKey::keys() const {
    if (!this->is_peer_map()) {
        RAISE_ERROR("PublicKey::keys() called on a single-key instance");
    }
    return this->peer_keys;
}
