#pragma once

#include <string>
#include <vector>

#include "LinkSchema.h"

using namespace commons;
using namespace atoms;

namespace atomdb {

enum class AuthorizationOperation { READ, WRITE };

class AuthorizationSchema {
   public:
    AuthorizationSchema(const LinkSchema& schema, bool read, bool write);
    AuthorizationSchema(const vector<string>& tokens, bool read, bool write);
    ~AuthorizationSchema() = default;

    bool allows(AuthorizationOperation operation) const;
    const LinkSchema& schema() const;

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

    bool is_full_access() const;
    const vector<AuthorizationSchema>& schemas() const;

   private:
    string access_key_;
    bool full_access_;
    vector<AuthorizationSchema> schemas_;
};

}  // namespace atomdb