#include "AuthorizationTypes.h"

#include "AtomDBAPITypes.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;
using namespace commons;
using namespace atoms;

AuthorizationSchema::AuthorizationSchema(const LinkSchema& schema, bool read, bool write)
    : schema_(schema), read_(read), write_(write) {
    if (!read && !write) {
        RAISE_ERROR("At least one of read or write must be true");
    }
}

AuthorizationSchema::AuthorizationSchema(const vector<string>& tokens, bool read, bool write)
    : schema_(tokens), read_(read), write_(write) {
    if (!read && !write) {
        RAISE_ERROR("At least one of read or write must be true");
    }
}

bool AuthorizationSchema::allows(AuthorizationOperation operation) const {
    switch (operation) {
        case AuthorizationOperation::READ:
            return this->read_;
        case AuthorizationOperation::WRITE:
            return this->write_;
    }
    return false;
}

const LinkSchema& AuthorizationSchema::schema() const { return this->schema_; }

bool AuthorizationSchema::read() const { return this->read_; }

bool AuthorizationSchema::write() const { return this->write_; }

bool AuthorizationSchema::match(const Atom& atom, HandleDecoder& decoder) const {
    Assignment assignment;
    LinkSchema schema = this->schema_;
    if (Atom::is_link(atom)) {
        auto& link = *(Link*) &static_cast<const Link&>(atom);
        return schema.match(link, assignment, decoder);
    }
    return schema.match(atom.handle(), assignment, decoder);
}

bool AuthorizationSchema::match(const string& handle, HandleDecoder& decoder) const {
    Assignment assignment;
    LinkSchema schema = this->schema_;
    return schema.match(handle, assignment, decoder);
}

AuthorizationProfile::AuthorizationProfile(const string& access_key,
                                           bool full_access,
                                           vector<AuthorizationSchema> schemas)
    : access_key_(access_key), full_access_(full_access), schemas_(schemas) {
    if (full_access && !schemas.empty()) {
        RAISE_ERROR("schemas must be empty when full_access is true");
    }
}

AuthorizationProfile AuthorizationProfile::from_document(
    const atomdb_api_types::AccessPermissionDocument& document) {
    vector<AuthorizationSchema> schemas;
    schemas.reserve(document.get_entries_size());
    for (unsigned int i = 0; i < document.get_entries_size(); ++i) {
        const auto& entry = document.get_entry(i);
        vector<string> tokens;
        tokens.reserve(entry.get_tokens_size());
        for (unsigned int j = 0; j < entry.get_tokens_size(); ++j) {
            tokens.push_back(entry.get_token(j));
        }
        schemas.emplace_back(tokens, entry.get_read(), entry.get_write());
    }
    return AuthorizationProfile(document.get_access_key(), document.get_full_access(), move(schemas));
}

const string& AuthorizationProfile::access_key() const { return this->access_key_; }

bool AuthorizationProfile::is_full_access() const { return this->full_access_; }

const vector<AuthorizationSchema>& AuthorizationProfile::schemas() const { return this->schemas_; }

AuthorizationProfile AuthorizationProfile::with_schema(const AuthorizationSchema& schema) const {
    if (this->full_access_) {
        RAISE_ERROR("Cannot add authorization schemas to a full_access profile");
    }

    vector<AuthorizationSchema> schemas;
    schemas.reserve(this->schemas_.size() + 1);
    bool replaced = false;
    for (const auto& existing : this->schemas_) {
        if (existing.schema().handle() == schema.schema().handle()) {
            schemas.push_back(schema);
            replaced = true;
        } else {
            schemas.push_back(existing);
        }
    }
    if (!replaced) {
        schemas.push_back(schema);
    }
    return AuthorizationProfile(this->access_key_, this->full_access_, move(schemas));
}

optional<AuthorizationProfile> AuthorizationProfile::without_schema(
    const AuthorizationSchema& schema) const {
    vector<AuthorizationSchema> schemas;
    schemas.reserve(this->schemas_.size());
    for (const auto& existing : this->schemas_) {
        if (existing.schema().handle() != schema.schema().handle()) {
            schemas.push_back(existing);
        }
    }
    if (schemas.size() == this->schemas_.size()) {
        return *this;
    }
    return AuthorizationProfile(this->access_key_, this->full_access_, move(schemas));
}
