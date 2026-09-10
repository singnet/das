#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "AtomDB.h"
#include "AtomDBAPITypes.h"
#include "LinkSchema.h"

using namespace commons;
using namespace atoms;
using namespace std;
using namespace atomdb;

namespace atomdb {

enum class AuthorizationOperation { READ, WRITE };

class AuthorizationSchema {
   public:
    AuthorizationSchema(shared_ptr<AtomDB> atomdb, const LinkSchema& schema, bool read, bool write);
    AuthorizationSchema(shared_ptr<AtomDB> atomdb, const vector<string>& tokens, bool read, bool write);
    ~AuthorizationSchema() = default;

    bool is_granted(shared_ptr<Atom> atom, AuthorizationOperation operation);
    bool allows(AuthorizationOperation operation) const;

    // These method are used only for testing purposes.
   protected:
    inline const LinkSchema& schema() const { return this->schema_; }

   private:
    shared_ptr<AtomDB> atomdb_;
    LinkSchema schema_;
    bool read_;
    bool write_;
};

class AuthorizationProfile {
   public:
    AuthorizationProfile(bool full_access, vector<shared_ptr<AuthorizationSchema>> schemas);
    ~AuthorizationProfile() = default;

    /**
     * @brief Returns an AuthorizationProfile built from an AccessPermissionDocument.
     */
    static shared_ptr<AuthorizationProfile> from_document(
        shared_ptr<AtomDB> atomdb,
        const shared_ptr<atomdb_api_types::AccessPermissionDocument>& document);

    /**
     * @brief Returns whether this profile grants unrestricted access.
     */
    inline bool is_unrestricted() const { return this->full_access_; }

    bool is_granted(shared_ptr<Atom> atom, AuthorizationOperation operation);

    // These method are used only for testing purposes.
   protected:
    const vector<shared_ptr<AuthorizationSchema>> schemas() const { return this->schemas_; }

   private:
    bool full_access_;
    vector<shared_ptr<AuthorizationSchema>> schemas_;
};

class Keychain {
   public:
    using AtomDB_UID = string;
    using PublicKey = string;

    explicit Keychain(map<AtomDB_UID, PublicKey> keys);
    ~Keychain() = default;

    inline bool empty() const { return this->keys_.empty(); }
    inline const map<AtomDB_UID, PublicKey>& keys() const { return this->keys_; }

    PublicKey get(const AtomDB_UID& uid) const;

   private:
    map<AtomDB_UID, PublicKey> keys_;
};

}  // namespace atomdb
