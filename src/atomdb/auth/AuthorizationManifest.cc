#include "AuthorizationManifest.h"

#include <algorithm>

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;

// --------------------------------------------------------------------------------
// Public methods

bool AuthorizationManifest::is_authorized(const Atom& atom,
                                          const string& public_key,
                                          AuthorizationOperation operation,
                                          HandleDecoder& decoder) {
    if (!this->is_registered(public_key)) {
        return false;
    }
    if (this->full_access(public_key)) {
        return true;
    }

    auto profile = this->lookup(public_key);
    for (const auto& schema : profile->schemas()) {
        if (schema.allows(operation) && schema.match(atom, decoder)) {
            return true;
        }
    }
    return false;
}

bool AuthorizationManifest::is_authorized(const string& handle,
                                          const string& public_key,
                                          AuthorizationOperation operation,
                                          HandleDecoder& decoder) {
    if (!this->is_registered(public_key)) {
        return false;
    }
    if (this->full_access(public_key)) {
        return true;
    }

    auto profile = this->lookup(public_key);
    for (const auto& schema : profile->schemas()) {
        if (schema.allows(operation) && schema.match(handle, decoder)) {
            return true;
        }
    }
    return false;
}

bool AuthorizationManifest::is_registered(const string& public_key) const {
    return this->profiles.find(public_key) != this->profiles.end();
}

bool AuthorizationManifest::full_access(const string& public_key) {
    auto profile = this->lookup(public_key);
    if (profile == nullptr) return false;
    return profile->is_full_access();
}

AuthorizationProfile* AuthorizationManifest::lookup(const string& public_key) {
    auto it = this->profiles.find(public_key);

    if (it == this->profiles.end()) {
        return nullptr;
    }
    return &it->second;
}

void AuthorizationManifest::add_document(
    const shared_ptr<atomdb_api_types::AccessPermissionDocument>& document) {
    vector<AuthorizationSchema> schemas;
    schemas.reserve(document->get_entries_size());

    for (unsigned int i = 0; i < document->get_entries_size(); ++i) {
        const auto& entry = document->get_entry(i);
        vector<string> tokens;
        tokens.reserve(entry.get_tokens_size());
        for (unsigned int j = 0; j < entry.get_tokens_size(); ++j) {
            tokens.push_back(entry.get_token(j));
        }
        schemas.emplace_back(tokens, entry.get_read(), entry.get_write());
    }

    auto [it, inserted] = this->profiles.emplace(
        document->get_access_key(),
        AuthorizationProfile(
            document->get_access_key(), document->get_full_access(), std::move(schemas)));
    if (!inserted) {
        RAISE_ERROR(string("Duplicate access_key in authorization manifest: ") +
                    document->get_access_key());
    }
}
