#include "AuthorizationManifest.h"

#include <algorithm>

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"

using namespace atomdb;

// --------------------------------------------------------------------------------
// Public methods

void AuthorizationManifest::set(const atomdb_api_types::AccessPermissionDocument& document) {
    auto it = this->documents.find(document.access_key);
    if (it == this->documents.end()) {
        this->documents.emplace(document.access_key,
                                atomdb_api_types::AccessPermissionDocument(document));
    } else {
        it->second = document;
    }
}

void AuthorizationManifest::add(const string& public_key,
                                const atomdb_api_types::AccessPermissionEntry& entry) {
    auto document = this->get_document(public_key);

    if (document == nullptr) {
        this->documents.emplace(public_key,
                                atomdb_api_types::AccessPermissionDocument(public_key, false, {entry}));
        return;
    }

    vector<atomdb_api_types::AccessPermissionEntry>& entries = document->entries;

    for (auto& existing : entries) {
        if (existing.schema.handle() == entry.schema.handle()) {
            existing = entry;
            return;
        }
    }

    entries.push_back(entry);
}

void AuthorizationManifest::remove(const string& public_key,
                                   const atomdb_api_types::AccessPermissionEntry& entry) {
    auto document = this->get_document(public_key);

    if (document == nullptr) return;

    vector<atomdb_api_types::AccessPermissionEntry>& entries = document->entries;

    for (auto it = entries.begin(); it != entries.end(); ++it) {
        if (it->schema.handle() == entry.schema.handle()) {
            entries.erase(it);
            return;
        }
    }
}

void AuthorizationManifest::remove_all(const string& public_key) { this->documents.erase(public_key); }

bool AuthorizationManifest::is_registered(const string& public_key) const {
    return this->documents.find(public_key) != this->documents.end();
}

bool AuthorizationManifest::full_access(const string& public_key) {
    auto document = this->get_document(public_key);
    if (document == nullptr) return false;
    return document->full_access;
}

atomdb_api_types::AccessPermissionDocument* AuthorizationManifest::get_document(
    const string& public_key) {
    auto it = this->documents.find(public_key);

    if (it == this->documents.end()) {
        return nullptr;
    }
    return &it->second;
}
