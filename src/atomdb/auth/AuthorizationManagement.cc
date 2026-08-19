#include "AuthorizationManagement.h"

#include "Assignment.h"
#include "Atom.h"
#include "AtomDB.h"
#include "Link.h"
#include "Utils.h"

using namespace atomdb;
using namespace atoms;
using namespace commons;

// --------------------------------------------------------------------------------
// Constructors

AuthorizationManagement::AuthorizationManagement(shared_ptr<AuthorizationPersistence> persistence)
    : persistence(persistence) {
    if (this->persistence == nullptr) {
        RAISE_ERROR("AuthorizationManagement requires a non-null persistence");
    }
}

// --------------------------------------------------------------------------------
// Public methods

bool AuthorizationManagement::has_full_access(const string& public_key) {
    return this->manifest.is_registered(public_key) && this->manifest.full_access(public_key);
}

bool AuthorizationManagement::is_authorized(const Atom& atom,
                                            const string& public_key,
                                            AuthorizationOperation operation,
                                            HandleDecoder& decoder) {
    if (!this->manifest.is_registered(public_key)) {
        return false;
    }
    if (this->manifest.full_access(public_key)) {
        return true;
    }

    auto document = this->manifest.get_document(public_key);
    for (const auto& entry : document->entries) {
        if (this->allows(entry, operation) && this->matches_schema(entry.schema, atom, decoder)) {
            return true;
        }
    }
    return false;
}

bool AuthorizationManagement::is_authorized(const string& handle,
                                            const string& public_key,
                                            AuthorizationOperation operation,
                                            HandleDecoder& decoder) {
    if (!this->manifest.is_registered(public_key)) {
        return false;
    }
    if (this->manifest.full_access(public_key)) {
        return true;
    }

    auto document = this->manifest.get_document(public_key);
    for (const auto& entry : document->entries) {
        if (allows(entry, operation) && this->matches_schema(entry.schema, handle, decoder)) {
            return true;
        }
    }
    return false;
}

void AuthorizationManagement::authorize(const string& public_key,
                                        const atomdb_api_types::AccessPermissionEntry& entry) {
    this->persistence->save(public_key, entry);
    this->manifest.add(public_key, entry);
}

void AuthorizationManagement::revoke(const string& public_key,
                                     const atomdb_api_types::AccessPermissionEntry& entry) {
    this->persistence->remove(public_key, entry);
    this->manifest.remove(public_key, entry);
}

void AuthorizationManagement::revoke_all(const string& public_key) {
    this->persistence->remove_all(public_key);
    this->manifest.remove_all(public_key);
}

// --------------------------------------------------------------------------------
// Private methods

bool AuthorizationManagement::allows(const atomdb_api_types::AccessPermissionEntry& entry,
                                     AuthorizationOperation operation) {
    switch (operation) {
        case AuthorizationOperation::READ:
            return entry.read;
        case AuthorizationOperation::WRITE:
            return entry.write;
    }
    return false;
}

// ???
bool AuthorizationManagement::matches_schema(const LinkSchema& schema,
                                             const Atom& atom,
                                             HandleDecoder& decoder) const {
    Assignment assignment;
    LinkSchema local_schema(schema);
    if (Atom::is_link(atom)) {
        return local_schema.match(
            const_cast<Link&>(static_cast<const Link&>(atom)), assignment, decoder);
    }
    return local_schema.match(atom.handle(), assignment, decoder);
}

bool AuthorizationManagement::matches_schema(const LinkSchema& schema,
                                             const string& handle,
                                             HandleDecoder& decoder) const {
    Assignment assignment;
    LinkSchema local_schema(schema);
    return local_schema.match(handle, assignment, decoder);
}
