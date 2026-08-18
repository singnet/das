#include "AuthorizationManifest.h"

#include <algorithm>

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"

using namespace atomdb;
using namespace auth;

const vector<atomdb_api_types::AccessPermissionEntry> AuthorizationManifest::EMPTY_ENTRIES;

// --------------------------------------------------------------------------------
// Public methods

void AuthorizationManifest::set(const atomdb_api_types::AccessPermissionDocument& document) {
    auto it = this->documents.find(document.public_key);
    if (it == this->documents.end()) {
        this->documents.emplace(document.public_key, document);
    } else {
        it->second = document;
    }
}

void AuthorizationManifest::add(const atomdb_api_types::PublicKey& public_key,
                                const atomdb_api_types::AccessPermissionEntry& entry) {
    atomdb_api_types::AccessPermissionDocument* document = this->find_document(public_key, "add");

    if (document == nullptr) {
        this->create_document(public_key, entry);
        return;
    }

    string entry_handle = entry.schema.handle();
    vector<atomdb_api_types::AccessPermissionEntry>& entries = document->entries;

    for (auto& existing : entries) {
        if (existing.schema.handle() == entry_handle) {
            existing = entry;
            return;
        }
    }

    entries.push_back(entry);
}

void AuthorizationManifest::remove(const atomdb_api_types::PublicKey& public_key, const string& handle) {
    atomdb_api_types::AccessPermissionDocument* document = this->find_document(public_key, "remove");

    if (document == nullptr) return;

    vector<atomdb_api_types::AccessPermissionEntry>& entries = document->entries;

    for (auto it = entries.begin(); it != entries.end(); ++it) {
        if (it->schema.handle() == handle) {
            entries.erase(it);
            return;
        }
    }
}

void AuthorizationManifest::remove_all(const atomdb_api_types::PublicKey& public_key) {
    this->documents.erase(public_key);
}

bool AuthorizationManifest::is_registered(const atomdb_api_types::PublicKey& public_key) const {
    return this->documents.find(public_key) != this->documents.end();
}

bool AuthorizationManifest::full_access(const atomdb_api_types::PublicKey& public_key) {
    atomdb_api_types::AccessPermissionDocument* document =
        this->find_document(public_key, "full_access");
    if (document == nullptr) {
        return false;
    }
    return document->full_access;
}

const vector<atomdb_api_types::AccessPermissionEntry>& AuthorizationManifest::entries(
    const atomdb_api_types::PublicKey& public_key) {
    atomdb_api_types::AccessPermissionDocument* document = this->find_document(public_key, "entries");
    if (document == nullptr) {
        return EMPTY_ENTRIES;
    }
    return document->entries;
}

// --------------------------------------------------------------------------------
// Private methods

void AuthorizationManifest::create_document(const atomdb_api_types::PublicKey& public_key,
                                            const atomdb_api_types::AccessPermissionEntry& entry) {
    this->documents.emplace(public_key,
                            atomdb_api_types::AccessPermissionDocument(public_key, false, {entry}));
}

atomdb_api_types::AccessPermissionDocument* AuthorizationManifest::find_document(
    const atomdb_api_types::PublicKey& public_key, const string& caller) {
    auto it = this->documents.find(public_key);
    if (it == this->documents.end()) {
        LOG_INFO("AuthorizationManifest::" << caller
                                           << "() called for unregistered public_key: " << public_key);
        return nullptr;
    }
    return &it->second;
}
