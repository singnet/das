#pragma once

#include <map>
#include <string>
#include <vector>

#include "AuthorizationEntry.h"

using namespace std;

namespace atomdb {

/**
 * @brief In-RAM image of the whole access_permissions collection.
 *
 * AuthorizationManagement keeps exactly one of these. Document maps 1:1 to one MongoDB document.
 */
class AuthorizationManifest {
   public:
    /** @brief One access_permissions document. */
    class Document {
       public:
        string public_key;
        bool full_access = false;
        vector<AuthorizationEntry> entries;
    };

    AuthorizationManifest() = default;

    /** @brief Replaces (or inserts) the document for document.public_key. */
    void set(const Document& document);

    /** @brief Adds one entry to public_key, creating the document if needed. */
    void add(const string& public_key, const AuthorizationEntry& entry);

    /** @brief Removes the entry whose handle() == handle from public_key. No-op if absent. */
    void remove(const string& public_key, const string& handle);

    /** @brief Removes the whole document for public_key. No-op if not registered. */
    void remove_all(const string& public_key);

    /** @brief Returns true if public_key is registered. */
    bool is_registered(const string& public_key) const;

    /** @brief Returns true if public_key is registered with full_access. */
    bool full_access(const string& public_key);

    /** @brief Returns the entries for public_key, or an empty vector if not registered. */
    const vector<AuthorizationEntry>& entries(const string& public_key);

   private:
    map<string, Document> documents;
    static const vector<AuthorizationEntry> EMPTY_ENTRIES;

    void create_document(const string& public_key, const AuthorizationEntry& entry);
    Document* find_document(const string& public_key, const string& caller);
};

}  // namespace atomdb
