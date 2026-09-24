#include "AuthorizationManifest.h"

#include <algorithm>

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;

// -------------------------------------------------------------------------------------------------
// Constructor

AuthorizationManifest::AuthorizationManifest(shared_ptr<AtomDB> atomdb) : atomdb(atomdb) {
    if (this->atomdb == nullptr) {
        RAISE_ERROR("AuthorizationManifest requires a non-null AtomDB");
    }
}

// --------------------------------------------------------------------------------
// Public methods

bool AuthorizationManifest::is_granted(const string& public_key,
                                       shared_ptr<Atom> atom,
                                       AuthorizationOperation operation) {
    if (atom == nullptr) {
        RAISE_ERROR("AuthorizationManifest::is_granted() requires a non-null atom or a valid handle");
    }
    return this->is_granted(public_key, operation, atom, "");
}

bool AuthorizationManifest::is_granted(const string& public_key,
                                       const string& handle,
                                       AuthorizationOperation operation) {
    return this->is_granted(public_key, operation, nullptr, handle);
}

bool AuthorizationManifest::ensure_profile_loaded(const string& public_key) {
    if (public_key.empty()) return false;

    lock_guard<mutex> lock(this->profiles_mutex);

    if (this->profiles.find(public_key) != this->profiles.end()) {
        return true;
    }

    auto access_document = this->atomdb->get_access_permissions(public_key);
    if (access_document == nullptr) {
        return false;
    }

    string access_key = access_document->get_access_key();
    if (access_key != public_key) {
        return false;
    }

    this->profiles.emplace(access_key,
                           AuthorizationProfile::from_document(this->atomdb, access_document));

    return true;
}

// --------------------------------------------------------------------------------
// Private methods

bool AuthorizationManifest::is_granted(const string& public_key,
                                       AuthorizationOperation operation,
                                       shared_ptr<Atom> atom,
                                       const string& handle) {
    shared_ptr<AuthorizationProfile> profile;
    {
        lock_guard<mutex> lock(this->profiles_mutex);
        auto it = this->profiles.find(public_key);
        if (it == this->profiles.end() || it->second == nullptr) return false;
        profile = it->second;
    }

    if (atom == nullptr) {
        atom = this->atomdb->get_atom(handle);
        if (atom == nullptr) return false;
    }
    return profile->is_granted(atom, operation);
}
