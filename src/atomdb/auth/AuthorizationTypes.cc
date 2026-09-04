#include "AuthorizationTypes.h"

#include "AtomDBAPITypes.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;
using namespace commons;
using namespace atoms;

/**
 * AuthorizationSchema
 */

// -------------------------------------------------------------------------------------------------
// Constructor

AuthorizationSchema::AuthorizationSchema(shared_ptr<AtomDB> atomdb,
                                         const LinkSchema& schema,
                                         bool read,
                                         bool write)
    : atomdb_(atomdb), schema_(schema), read_(read), write_(write) {
    if (!read && !write) {
        RAISE_ERROR("At least one of read or write must be true");
    }
}

AuthorizationSchema::AuthorizationSchema(shared_ptr<AtomDB> atomdb,
                                         const vector<string>& tokens,
                                         bool read,
                                         bool write)
    : atomdb_(atomdb), schema_(tokens), read_(read), write_(write) {
    if (!read && !write) {
        RAISE_ERROR("At least one of read or write must be true");
    }
}

// -------------------------------------------------------------------------------------------------
// Public methods

bool AuthorizationSchema::is_authorized(shared_ptr<Atom> atom, AuthorizationOperation operation) {
    if (!this->allows(operation) || atom == nullptr) return false;

    Assignment assignment;
    shared_ptr<Link> link = dynamic_pointer_cast<Link>(atom);

    if (link != nullptr) {
        return this->schema_.match(*link, assignment, *this->atomdb_);
    } else {
        shared_ptr<Node> node = dynamic_pointer_cast<Node>(atom);
        if (node == nullptr) {
            RAISE_ERROR("Atom must be a Link or a Node.");
        }
        return this->schema_.match(node->handle(), assignment, *this->atomdb_);
    }
}

bool AuthorizationSchema::is_authorized(const string& handle, AuthorizationOperation operation) {
    if (!this->allows(operation)) return false;

    Assignment assignment;
    return this->schema_.match(handle, assignment, *this->atomdb_);
}

// -------------------------------------------------------------------------------------------------
// Private method

bool AuthorizationSchema::allows(AuthorizationOperation operation) const {
    switch (operation) {
        case AuthorizationOperation::READ:
            return this->read_;
        case AuthorizationOperation::WRITE:
            return this->write_;
    }
    return false;
}

/**
 * AuthorizationProfile
 */

// -------------------------------------------------------------------------------------------------
// Constructor

AuthorizationProfile::AuthorizationProfile(bool full_access,
                                           vector<shared_ptr<AuthorizationSchema>> schemas)
    : full_access_(full_access), schemas_(schemas) {
    if (full_access && !schemas.empty()) {
        RAISE_ERROR("schemas must be empty when full_access is true");
    }
}

// -------------------------------------------------------------------------------------------------
// Public method

shared_ptr<AuthorizationProfile> AuthorizationProfile::from_document(
    shared_ptr<AtomDB> atomdb, const shared_ptr<atomdb_api_types::AccessPermissionDocument>& document) {
    vector<shared_ptr<AuthorizationSchema>> schemas;
    schemas.reserve(document->get_entries_size());
    for (unsigned int i = 0; i < document->get_entries_size(); ++i) {
        const auto& entry = document->get_entry(i);
        vector<string> tokens;
        tokens.reserve(entry.get_tokens_size());
        for (unsigned int j = 0; j < entry.get_tokens_size(); ++j) {
            tokens.push_back(entry.get_token(j));
        }
        schemas.emplace_back(
            make_shared<AuthorizationSchema>(atomdb, tokens, entry.get_read(), entry.get_write()));
    }
    return make_shared<AuthorizationProfile>(document->get_full_access(), move(schemas));
}

bool AuthorizationProfile::is_authorized(shared_ptr<Atom> atom, AuthorizationOperation operation) {
    for (const auto& schema : this->schemas_) {
        if (schema->is_authorized(atom, operation)) return true;
    }
    return false;
}

bool AuthorizationProfile::is_authorized(const string& handle, AuthorizationOperation operation) {
    for (const auto& schema : this->schemas_) {
        if (schema->is_authorized(handle, operation)) return true;
    }
    return false;
}