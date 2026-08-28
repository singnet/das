#pragma once

#include <optional>
#include <string>
#include <vector>

#include "LinkSchema.h"

using namespace commons;
using namespace atoms;

namespace atomdb {

namespace atomdb_api_types {
class AccessPermissionDocument;
}

enum class AuthorizationOperation { READ, WRITE };

class AuthorizationSchema {
   public:
    AuthorizationSchema(const LinkSchema& schema, bool read, bool write);
    AuthorizationSchema(const vector<string>& tokens, bool read, bool write);
    ~AuthorizationSchema() = default;

    bool allows(AuthorizationOperation operation) const;
    const LinkSchema& schema() const;
    bool read() const;
    bool write() const;
    bool match(const Atom& atom, HandleDecoder& decoder) const;
    bool match(const string& handle, HandleDecoder& decoder) const;

   private:
    LinkSchema schema_;
    bool read_;
    bool write_;
};

class AuthorizationProfile {
   public:
    AuthorizationProfile(const string& access_key,
                         bool full_access,
                         vector<AuthorizationSchema> schemas);
    ~AuthorizationProfile() = default;

    /**
     * @brief Returns an AuthorizationProfile built from an AccessPermissionDocument.
     */
    static AuthorizationProfile from_document(
        const atomdb_api_types::AccessPermissionDocument& document);

    /**
     * @brief Returns the access key associated with this profile.
     */
    const string& access_key() const;

    /**
     * @brief Returns whether this profile grants unrestricted access.
     */
    bool is_full_access() const;

    /**
     * @brief Returns the authorization schemas granted by this profile.
     */
    const vector<AuthorizationSchema>& schemas() const;

    /**
     * @brief Returns a copy with schema inserted or replaced by LinkSchema handle.
     */
    AuthorizationProfile with_schema(const AuthorizationSchema& schema) const;

    /**
     * @brief Returns a copy without schema, or nullopt when the last schema is removed.
     */
    std::optional<AuthorizationProfile> without_schema(const AuthorizationSchema& schema) const;

   private:
    string access_key_;
    bool full_access_;
    vector<AuthorizationSchema> schemas_;
};

}  // namespace atomdb