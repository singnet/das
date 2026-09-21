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

AuthorizationManifest::AuthorizationManifest(shared_ptr<AtomDB> atomdb) : atomdb(atomdb) {
    if (this->atomdb == nullptr) {
        RAISE_ERROR("AuthorizationManifest requires a non-null AtomDB");
    }
}

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
    shared_ptr<AuthorizationProfile> profile;
    {
        lock_guard<mutex> lock(this->profiles_mutex);
        auto it = this->profiles.find(public_key);
        if (it == this->profiles.end() || it->second == nullptr) return false;
        profile = it->second;
    }

    auto atom = this->atomdb->get_atom(handle);
    if (atom == nullptr) return false;

    return profile->is_granted(atom, operation);
}

bool AuthorizationManifest::ensure_profile_loaded(const string& public_key) {
    if (public_key.empty()) return false;

    {
        lock_guard<mutex> lock(this->profiles_mutex);
        if (this->profiles.find(public_key) != this->profiles.end()) {
            return true;
        }
    }

    auto access_document = this->atomdb->get_access_permissions(public_key);
    if (access_document == nullptr) {
        return false;
    }

    string access_key = access_document->get_access_key();
    auto profile = AuthorizationProfile::from_document(this->atomdb, access_document);

    lock_guard<mutex> lock(this->profiles_mutex);
    bool inserted = this->profiles.emplace(access_key, profile).second;
    if (!inserted) {
        RAISE_ERROR(string("Duplicate access_key in authorization manifest: ") + access_key);
    }
    return true;
}
