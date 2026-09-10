#include "AuthorizationManifest.h"

#include <algorithm>

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;

// -------------------------------------------------------------------------------------------------
// Constructor

// --------------------------------------------------------------------------------
// Public methods

AuthorizationManifest::AuthorizationManifest(shared_ptr<AtomDB> atomdb) : atomdb(atomdb) {}

bool AuthorizationManifest::is_granted(const string& public_key,
                                       shared_ptr<Atom> atom,
                                       AuthorizationOperation operation) {
    if (!this->is_registered(public_key)) return false;

    auto it = this->profiles.find(public_key);
    if (it == this->profiles.end() || it->second == nullptr) return false;

    return it->second->is_granted(atom, operation);
}

bool AuthorizationManifest::is_granted(const string& public_key,
                                       const string& handle,
                                       AuthorizationOperation operation) {
    if (!this->is_registered(public_key)) return false;

    auto it = this->profiles.find(public_key);
    if (it == this->profiles.end() || it->second == nullptr) return false;

    auto atom = this->atomdb->get_atom(handle);
    if (atom == nullptr) return false;

    return it->second->is_granted(atom, operation);
}

void AuthorizationManifest::add_document(
    const shared_ptr<atomdb_api_types::AccessPermissionDocument>& document) {
    if (document == nullptr) {
        RAISE_ERROR("Authorization manifest document cannot be null");
    }

    auto [it, inserted] = this->profiles.emplace(
        document->get_access_key(), AuthorizationProfile::from_document(this->atomdb, document));
    if (!inserted) {
        RAISE_ERROR(string("Duplicate access_key in authorization manifest: ") +
                    document->get_access_key());
    }
}