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
    lock_guard<mutex> lock(this->profiles_mutex);
    auto it = this->profiles.find(public_key);
    if (it == this->profiles.end() || it->second == nullptr) return false;

    return it->second->is_granted(atom, operation);
}

bool AuthorizationManifest::is_granted(const string& public_key,
                                       const string& handle,
                                       AuthorizationOperation operation) {
    lock_guard<mutex> lock(this->profiles_mutex);
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

    lock_guard<mutex> lock(this->profiles_mutex);
    if (this->profiles.find(document->get_access_key()) != this->profiles.end()) {
        return;
    }
    this->profiles.emplace(document->get_access_key(),
                           AuthorizationProfile::from_document(this->atomdb, document));
}
