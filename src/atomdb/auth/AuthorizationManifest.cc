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

bool AuthorizationManifest::is_authorized(shared_ptr<Atom> atom,
                                          const string& public_key,
                                          AuthorizationOperation operation) {
    if (this->full_access(public_key)) return true;
    auto it = this->profiles.find(public_key);
    if (it == this->profiles.end() || it->second == nullptr) return false;
    return it->second->is_authorized(atom, operation);
}

bool AuthorizationManifest::is_authorized(const string& handle,
                                          const string& public_key,
                                          AuthorizationOperation operation) {
    if (this->full_access(public_key)) return true;
    auto it = this->profiles.find(public_key);
    if (it == this->profiles.end() || it->second == nullptr) return false;
    return it->second->is_authorized(handle, operation);
}

// -------------------------------------------------------------------------------------------------
// Protected method

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

// -------------------------------------------------------------------------------------------------
// Private method

bool AuthorizationManifest::full_access(const string& public_key) {
    if (!this->is_registered(public_key)) return false;
    auto profile = this->profiles[public_key];
    if (profile == nullptr) return false;
    return profile->is_full_access();
}
