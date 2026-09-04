#include "PublicKey.h"

#include <exception>
#include <nlohmann/json.hpp>

#include "Utils.h"

using json = nlohmann::json;
using namespace commons;

namespace atomdb {

PublicKey::PublicKey(const string& key) : keys{key} {
    if (key.empty()) {
        RAISE_ERROR("PublicKey cannot be empty");
    }
}

PublicKey::PublicKey(const map<string, string>& peer_keys) {
    if (peer_keys.empty()) {
        RAISE_ERROR("PublicKey cannot be empty");
    }
    this->keys.reserve(peer_keys.size());
    for (const auto& [peer, key] : peer_keys) {
        if (peer.empty() || key.empty()) {
            RAISE_ERROR("PublicKey peer and key must be non-empty");
        }
        this->peer_to_key[peer] = this->keys.size();
        this->keys.push_back(key);
    }
}

optional<string> PublicKey::key_for_peer(const string& peer_uid) const {
    if (this->is_single_key()) {
        return this->keys[0];
    }
    auto it = this->peer_to_key.find(peer_uid);
    if (it == this->peer_to_key.end()) {
        return nullopt;
    }
    if (it->second >= this->keys.size()) {
        RAISE_ERROR("PublicKey peer_to_key index out of range for peer: " + peer_uid);
    }
    return this->keys[it->second];
}

optional<PublicKey> PublicKey::for_peer(const string& peer_uid) const {
    if (this->is_single_key()) {
        return *this;
    }
    auto key = this->key_for_peer(peer_uid);
    if (!key.has_value()) {
        return nullopt;
    }
    return PublicKey(key.value());
}

map<string, string> PublicKey::peer_keys() const {
    map<string, string> result;
    for (const auto& [peer, idx] : this->peer_to_key) {
        if (idx >= this->keys.size()) {
            RAISE_ERROR("PublicKey peer_to_key index out of range for peer: " + peer);
        }
        result[peer] = this->keys[idx];
    }
    return result;
}

optional<PublicKey> PublicKey::from_json(const string& json_string) {
    json parsed;
    try {
        parsed = json::parse(json_string);
    } catch (const exception& exception) {
        RAISE_ERROR("Invalid PublicKey JSON: " + string(exception.what()));
    }

    if (parsed.is_string()) {
        return PublicKey(parsed.get<string>());
    }
    if (!parsed.is_object()) {
        RAISE_ERROR("PublicKey JSON must be an object or a string");
    }
    if (parsed.empty()) {
        return nullopt;
    }

    map<string, string> peer_keys;
    for (auto it = parsed.begin(); it != parsed.end(); ++it) {
        if (!it.value().is_string()) {
            RAISE_ERROR("PublicKey JSON values must be strings (peer '" + it.key() + "')");
        }
        peer_keys[it.key()] = it.value().get<string>();
    }
    return PublicKey(peer_keys);
}

}  // namespace atomdb
