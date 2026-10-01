#include "Keychain.h"

#include "Utils.h"

using namespace std;
using namespace atomdb;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Constructor

Keychain::Keychain(const map<string, string>& keys) { this->keys_ = keys; }

void Keychain::tokenize(vector<string>& tokens) {
    for (auto& pair : this->keys_) {
        tokens.push_back(pair.first);
        tokens.push_back(pair.second);
    }
}

void Keychain::untokenize(const vector<string>& tokens) {
    if (tokens.size() > 0) {
        bool parse_error = false;
        if ((tokens.size() % 2) == 0) {
            for (unsigned int i = 0; i < tokens.size(); i += 2) {
                if (tokens[i] != "") {
                    this->keys_[tokens[i]] = tokens[i + 1];
                } else {
                    parse_error = true;
                    break;
                }
            }
            if (this->keys_.size() != (tokens.size() / 2)) {
                parse_error = true;
            }
        } else {
            parse_error = true;
        }
        if (parse_error) {
            RAISE_ERROR(
                "Invalid tokens for public key. Expected a list of (uid, key) pairs (each uid being "
                "unique) in a string like 'uid1 key1 uid2 key2 ... uidn keyn' but got: <" +
                Utils::join(tokens) + ">");
        }
    }
}

// -------------------------------------------------------------------------------------------------
// Public methods

string Keychain::get_public_key(const string& uid) const {
    auto it = this->keys_.find(uid);
    if (it != this->keys_.end()) {
        return it->second;
    }
    return "";
}
