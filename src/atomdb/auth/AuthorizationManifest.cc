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
        LOG_ERROR("Corrupted or misplaced access document for key: " + public_key);
        return false;
    }

    this->profiles.emplace(access_key,
                           AuthorizationProfile::from_document(this->atomdb, access_document));

    return true;
}

void AuthorizationManifest::count_matching_atoms(size_t& node_count,
                                                 size_t& link_count,
                                                 size_t& atom_count,
                                                 shared_ptr<Keychain> keychain) {
    if (keychain == nullptr) {
        return;
    }

    string public_key = keychain->get_public_key(this->atomdb->get_uid());
    if (!this->ensure_profile_loaded(public_key)) {
        LOG_ERROR("No authorization profile found for public key: " + public_key);
        return;
    }

    shared_ptr<AuthorizationProfile> profile;
    {
        lock_guard<mutex> lock(this->profiles_mutex);
        profile = this->profiles.at(public_key);
    }

    if (profile->is_unrestricted()) {
        node_count += this->atomdb->node_count();
        link_count += this->atomdb->link_count();
        atom_count += this->atomdb->atom_count();
        return;
    }

    profile->count_matching_atoms(node_count, link_count, atom_count, keychain);
}

bool AuthorizationManifest::is_unrestricted(const string& public_key) {
    lock_guard<mutex> lock(this->profiles_mutex);
    auto it = this->profiles.find(public_key);
    if (it == this->profiles.end() || it->second == nullptr) return false;
    return it->second->is_unrestricted();
}

// --------------------------------------------------------------------------------
// Private methods

bool AuthorizationManifest::is_granted(const string& public_key,
                                       AuthorizationOperation operation,
                                       shared_ptr<Atom> atom,
                                       const string& handle) {
    lock_guard<mutex> lock(this->profiles_mutex);
    auto it = this->profiles.find(public_key);
    if (it == this->profiles.end() || it->second == nullptr) return false;

    if (atom == nullptr) {
        atom = this->atomdb->get_atom(handle);
        if (atom == nullptr) return false;
    }
    return it->second->is_granted(atom, operation);
}
