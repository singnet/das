#include "AuthorizationManifest.h"

#include <algorithm>

using namespace atomdb;

const vector<AuthorizationEntry> AuthorizationManifest::EMPTY_ENTRIES;

// --------------------------------------------------------------------------------
// Public methods

void AuthorizationManifest::set(const Document& document) {
    this->documents[document.public_key] = document;
}

void AuthorizationManifest::add(const string& public_key, const AuthorizationEntry& entry) {
    Document* document = this->find_document(public_key, "add");

    if (document == nullptr) {
        this->create_document(public_key, entry);
        return;
    }

    string entry_handle = entry.handle();
    vector<AuthorizationEntry>& entries = document->entries;

    for (auto& existing : entries) {
        if (existing.handle() == entry_handle) {
            existing = entry;
            return;
        }
    }

    entries.push_back(entry);
}

void AuthorizationManifest::remove(const string& public_key, const string& handle) {
    Document* document = this->find_document(public_key, "remove");

    if (document == nullptr) return;

    vector<AuthorizationEntry>& entries = document->entries;

    for (auto it = entries.begin(); it != entries.end(); ++it) {
        if (it->handle() == handle) {
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
    Document* document = this->find_document(public_key, "full_access");
    if (document == nullptr) {
        return false;
    }
    return document->full_access;
}

const vector<AuthorizationEntry>& AuthorizationManifest::entries(const string& public_key) {
    Document* document = this->find_document(public_key, "entries");
    if (document == nullptr) {
        return EMPTY_ENTRIES;
    }
    return document->entries;
}

// --------------------------------------------------------------------------------
// Private methods

void AuthorizationManifest::create_document(const string& public_key, const AuthorizationEntry& entry) {
    Document document;
    document.public_key = public_key;
    document.full_access = false;
    document.entries.push_back(entry);
    this->documents[public_key] = document;
}

AuthorizationManifest::Document* AuthorizationManifest::find_document(const string& public_key,
                                                                      const string& caller) {
    auto it = this->documents.find(public_key);
    if (it == this->documents.end()) {
        LOG_INFO("AuthorizationManifest::" + caller +
                 "() called for unregistered public_key: " + public_key);
        return nullptr;
    }
    return &it->second;
}
