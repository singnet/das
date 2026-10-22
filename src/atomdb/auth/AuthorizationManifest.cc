#define LOG_LEVEL DEBUG_LEVEL

#include "AuthorizationManifest.h"

#include <algorithm>

#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;
using namespace commons;

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

    StopWatch timer_fetch;
    // timer_fetch.start();
    // LOG_DEBUG("ensure_profile_loaded: cache-miss, fetching access document");
    auto access_document = this->atomdb->get_access_permissions(public_key);
    // timer_fetch.stop();
    if (access_document == nullptr) {
        // LOG_DEBUG("ensure_profile_loaded: no access document | time: " << timer_fetch.milliseconds()
                                                                    //   << "ms");
        return false;
    }

    string access_key = access_document->get_access_key();
    if (access_key != public_key) {
        LOG_ERROR("Corrupted or misplaced access document for key: " + public_key);
        return false;
    }

    StopWatch timer_build;
    // timer_build.start();
    this->profiles.emplace(access_key,
                           AuthorizationProfile::from_document(this->atomdb, access_document));
    // timer_build.stop();
    // LOG_DEBUG("ensure_profile_loaded: loaded full_access="
    //           << (access_document->get_full_access() ? "true" : "false")
    //           << " schemas=" << access_document->get_entries_size()
    //           << " | fetch: " << timer_fetch.milliseconds() << "ms"
    //           << " | build: " << timer_build.milliseconds() << "ms");

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

bool atomdb::AuthorizationManifest::is_unrestricted(const string& public_key) {
    lock_guard<mutex> lock(this->profiles_mutex);
    auto it = this->profiles.find(public_key);
    if (it == this->profiles.end() || it->second == nullptr) {
        return false;
    }
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
    if (it == this->profiles.end() || it->second == nullptr) {
        // LOG_DEBUG("is_granted(" << handle << "): no profile");
        return false;
    }

    unsigned long fetch_ms = 0;
    if (atom == nullptr) {
        // StopWatch timer_fetch;
        // timer_fetch.start();
        atom = this->atomdb->get_atom(handle);
        // timer_fetch.stop();
        // fetch_ms = timer_fetch.milliseconds();
        if (atom == nullptr) {
            // LOG_DEBUG("is_granted(" << handle << "): atom not found | fetch: " << fetch_ms << "ms");
            return false;
        }
    }

    // StopWatch timer_check;
    // timer_check.start();
    bool granted = it->second->is_granted(atom, operation);
    // timer_check.stop();
    // LOG_DEBUG("is_granted(" << atom->handle() << "): " << (granted ? "granted" : "denied")
    //                         << " | fetch: " << fetch_ms << "ms"
    //                         << " | check: " << timer_check.milliseconds() << "ms");
    return granted;
}
