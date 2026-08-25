#include "AuthorizationManifest.h"

#include <algorithm>

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;

// --------------------------------------------------------------------------------
// Public methods

AuthorizationManifest::AuthorizationManifest(
    const vector<atomdb_api_types::AccessPermissionDocument>& documents) {
    for (const auto& doc : documents) {
        vector<AuthorizationSchema> schemas;
        schemas.reserve(doc.entries.size());

        for (const auto& entry : doc.entries) {
            schemas.emplace_back(entry.schema, entry.read, entry.write);
        }

        auto [it, inserted] = this->profiles.emplace(
            doc.access_key, AuthorizationProfile(doc.access_key, doc.full_access, std::move(schemas)));
        if (!inserted) {
            RAISE_ERROR("Duplicate access_key in authorization manifest: " + doc.access_key);
        }
    }
}

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
    for (const auto& rule : profile->schemas()) {
        if (rule.allows(operation) && this->matches_schema(rule.schema(), atom, decoder)) {
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
    for (const auto& rule : profile->schemas()) {
        if (rule.allows(operation) && this->matches_schema(rule.schema(), handle, decoder)) {
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

// --------------------------------------------------------------------------------
// Private methods

bool AuthorizationManifest::matches_schema(const LinkSchema& schema,
                                           const Atom& atom,
                                           HandleDecoder& decoder) const {
    Assignment assignment;
    LinkSchema local_schema(schema);
    if (Atom::is_link(atom)) {
        auto& link = const_cast<Link&>(static_cast<const Link&>(atom));
        return local_schema.match(link, assignment, decoder);
    }
    return local_schema.match(atom.handle(), assignment, decoder);
}

bool AuthorizationManifest::matches_schema(const LinkSchema& schema,
                                           const string& handle,
                                           HandleDecoder& decoder) const {
    Assignment assignment;
    LinkSchema local_schema(schema);
    return local_schema.match(handle, assignment, decoder);
}
