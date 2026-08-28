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
    if (document == nullptr) {
        RAISE_ERROR("Authorization manifest document cannot be null");
    }

    auto [it, inserted] = this->profiles.emplace(document->get_access_key(),
                                                 AuthorizationProfile::from_document(*document));
    if (!inserted) {
        RAISE_ERROR(string("Duplicate access_key in authorization manifest: ") +
                    document->get_access_key());
    }
}
