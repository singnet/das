#include "AuthorizationManagement.h"

#include "Assignment.h"
#include "AtomDB.h"
#include "Link.h"
#include "Utils.h"

using namespace atomdb;
using namespace atoms;
using namespace commons;

// --------------------------------------------------------------------------------
// Constructors

AuthorizationManagement::AuthorizationManagement(shared_ptr<AtomDB> atomdb,
                                                 shared_ptr<AuthorizationPersistence> persistence)
    : atomdb(std::move(atomdb)), persistence(std::move(persistence)) {
    if (this->atomdb == nullptr) {
        RAISE_ERROR("AuthorizationManagement requires a non-null atomdb AtomDB");
    }
    // Bootstrap from AtomDB::get_access_permissions() once that API exists on AtomDB.
    // Until then the in-RAM manifest starts empty and is filled only by authorize().
}

// --------------------------------------------------------------------------------
// Public methods

bool AuthorizationManagement::has_full_access(const string& public_key) {
    return this->manifest.is_registered(public_key) && this->manifest.full_access(public_key);
}

bool AuthorizationManagement::is_authorized(const Atom& atom,
                                            const string& public_key,
                                            AuthorizationOperation operation) {
    if (!this->manifest.is_registered(public_key)) {
        return false;
    }
    if (this->manifest.full_access(public_key)) {
        return true;
    }

    HandleDecoder& decoder = *this->atomdb;
    for (const auto& entry : this->manifest.entries(public_key)) {
        if (entry.allows(operation) && this->matches_entry(entry, atom, decoder)) {
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

    for (const auto& entry : this->manifest.entries(public_key)) {
        if (entry.allows(operation) && this->matches_entry(entry, handle, decoder)) {
            return true;
        }
    }
    return false;
}

void AuthorizationManagement::authorize(const string& public_key, const AuthorizationEntry& entry) {
    if (this->persistence == nullptr) {
        RAISE_ERROR(
            "AuthorizationManagement::authorize() requires AuthorizationPersistence; "
            "this atomdb has no authorization storage");
    }
    this->persistence->save(public_key, entry);
    this->manifest.add(public_key, entry);
}

void AuthorizationManagement::revoke(const string& public_key, const string& handle) {
    if (this->persistence == nullptr) {
        RAISE_ERROR(
            "AuthorizationManagement::revoke() requires AuthorizationPersistence; "
            "this atomdb has no authorization storage");
    }
    this->persistence->remove(public_key, handle);
    this->manifest.remove(public_key, handle);
}

void AuthorizationManagement::revoke_all(const string& public_key) {
    if (this->persistence == nullptr) {
        RAISE_ERROR(
            "AuthorizationManagement::revoke_all() requires AuthorizationPersistence; "
            "this atomdb has no authorization storage");
    }
    this->persistence->remove_all(public_key);
    this->manifest.remove_all(public_key);
}

// --------------------------------------------------------------------------------
// Private methods

bool AuthorizationManagement::matches_entry(const AuthorizationEntry& entry,
                                            const Atom& atom,
                                            HandleDecoder& decoder) const {
    Assignment assignment;
    LinkSchema schema = entry.schema();
    // Prefer matching against the in-memory atom so WRITE checks work for atoms not yet stored.
    if (Atom::is_link(atom)) {
        return schema.match(const_cast<Link&>(static_cast<const Link&>(atom)), assignment, decoder);
    }
    return schema.match(atom.handle(), assignment, decoder);
}

bool AuthorizationManagement::matches_entry(const AuthorizationEntry& entry,
                                            const string& handle,
                                            HandleDecoder& decoder) const {
    Assignment assignment;
    LinkSchema schema = entry.schema();
    return schema.match(handle, assignment, decoder);
}
