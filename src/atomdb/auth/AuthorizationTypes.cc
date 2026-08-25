#include "AuthorizationTypes.h"

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
    : schema_(tokens), read_(read), write_(write) {}

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
    if (!full_access && schemas.empty()) {
        RAISE_ERROR("schemas must be non-empty when full_access is false");
    }
}

bool AuthorizationProfile::is_full_access() const { return this->full_access_; }

const vector<AuthorizationSchema>& AuthorizationProfile::schemas() const { return this->schemas_; }
