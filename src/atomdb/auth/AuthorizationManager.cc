
#include "AuthorizationManager.h"

using namespace atomdb;

// --------------------------------------------------------------------------------
// Constructor

AuthorizationManager::AuthorizationManager(shared_ptr<AuthorizationPersistence> persistence)
    : persistence(persistence) {
    if (!this->persistence) {
        RAISE_ERROR("Authorization persistence is required");
    }
}

// --------------------------------------------------------------------------------
// Public methods

vector<atomdb_api_types::AccessPermissionEntry> atomdb::AuthorizationManager::list(
    const string& public_key) {
    return this->persistence->list(public_key);
}

void AuthorizationManager::authorize(const string& public_key,
                                     const atomdb_api_types::AccessPermissionEntry& entry) {
    this->persistence->save(public_key, entry);
}

void AuthorizationManager::revoke(const string& public_key,
                                  const atomdb_api_types::AccessPermissionEntry& entry) {
    this->persistence->remove(public_key, entry);
}

void AuthorizationManager::revoke_all(const string& public_key) {
    this->persistence->remove_all(public_key);
}
