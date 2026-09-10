#include "AuthorizationManifest.h"

#include <algorithm>

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;

// -------------------------------------------------------------------------------------------------
// Constructor

AuthorizationManifest::AuthorizationManifest(shared_ptr<AtomDB> atomdb) : atomdb(atomdb) {}

// --------------------------------------------------------------------------------
// Public methods

bool AuthorizationManifest::is_granted(const Keychain& keychain,
                                       shared_ptr<Atom> atom,
                                       AuthorizationOperation operation) {
    // TODO: Uncomment the code below once AtomdB::get_uid() is implemented
    // auto public_key = keychain.get_public_key(this->atomdb->get_uid());
    // if (public_key.empty()) return false;
    string public_key = "public_key";
    auto it = this->profiles.find(public_key);
    if (it == this->profiles.end() || it->second == nullptr) return false;

    return it->second->is_granted(atom, operation);
}

bool AuthorizationManifest::is_granted(const Keychain& keychain,
                                       const string& handle,
                                       AuthorizationOperation operation) {
    // TODO: Uncomment the code below once AtomdB::get_uid() is implemented
    // auto public_key = keychain.get_public_key(this->atomdb->get_uid());
    // if (public_key.empty()) return false;
    string public_key = "public_key";
    auto it = this->profiles.find(public_key);
    if (it == this->profiles.end() || it->second == nullptr) return false;

    auto atom = this->atomdb->get_atom(handle);
    if (atom == nullptr) return false;

    return it->second->is_granted(atom, operation);
}

void AuthorizationManifest::add_document(
    const shared_ptr<atomdb_api_types::AccessPermissionDocument>& document) {
    if (document == nullptr) {
        RAISE_ERROR("AuthorizationManifest document cannot be null");
    }

    string access_key = document->get_access_key();

    if (this->is_registered(access_key)) {
        RAISE_ERROR("Duplicate access_key in AuthorizationManifest: " + access_key);
    }

    auto profile = AuthorizationProfile::from_document(this->atomdb, document);

    this->profiles.emplace(access_key, profile);
}
