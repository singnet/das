#include "AuthorizationTypes.h"

using namespace std;
using namespace atomdb;
using namespace commons;
using namespace atoms;

AuthorizationSchema::AuthorizationSchema(const LinkSchema& schema, bool read, bool write)
    : schema_(schema), read_(read), write_(write) {}

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

AuthorizationProfile::AuthorizationProfile(const string& access_key,
                                           bool full_access,
                                           vector<AuthorizationSchema> schemas)
    : access_key_(access_key), full_access_(full_access), schemas_(schemas) {}

bool AuthorizationProfile::is_full_access() const { return this->full_access_; }

const vector<AuthorizationSchema>& AuthorizationProfile::schemas() const { return this->schemas_; }
