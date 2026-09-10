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

/**
 * @brief Operation checked during authorization.
 */
enum class AuthorizationOperation {
    READ,  ///< Read access to an Atom.
    WRITE  ///< Write access to an Atom.
};

/**
 * @brief A single schema-based permission rule.
 *
 * Binds a LinkSchema pattern to read and/or write access. During a grant check,
 * the schema matches the target atom against the pattern using the provided AtomDB.
 */
class AuthorizationSchema {
   public:
    /**
     * @brief Creates a schema rule from a LinkSchema and permission flags.
     *
     * @param atomdb AtomDB used to resolve atoms during pattern matching.
     * @param schema Link pattern that selects which atoms this rule applies to.
     * @param read Whether read access is granted when the pattern matches.
     * @param write Whether write access is granted when the pattern matches.
     * @throws std::runtime_error if both read and write are false.
     */
    AuthorizationSchema(shared_ptr<AtomDB> atomdb, const LinkSchema& schema, bool read, bool write);

    /**
     * @brief Creates a schema rule from LinkSchema tokens and permission flags.
     *
     * @param atomdb AtomDB used to resolve atoms during pattern matching.
     * @param tokens Token sequence that defines the LinkSchema pattern.
     * @param read Whether read access is granted when the pattern matches.
     * @param write Whether write access is granted when the pattern matches.
     * @throws std::runtime_error if both read and write are false.
     */
    AuthorizationSchema(shared_ptr<AtomDB> atomdb, const vector<string>& tokens, bool read, bool write);

    ~AuthorizationSchema() = default;

    /**
     * @brief Checks whether this schema grants the operation on the given atom.
     *
     * Returns false when the operation is not allowed, atom is null, or the
     * pattern does not match.
     *
     * @param atom Target atom to authorize.
     * @param operation Operation being requested.
     * @return true if this schema grants the operation on the atom.
     * @throws std::runtime_error if atom is non-null and is neither a Link nor a Node."
     */
    bool is_granted(shared_ptr<Atom> atom, AuthorizationOperation operation);

    /**
     * @brief Returns whether this schema allows the given operation type.
     *
     * @param operation Operation being requested.
     * @return true if this schema allows the operation type.
     * @throws std::runtime_error if operation is not READ or WRITE
     */
    bool allows(AuthorizationOperation operation) const;

   protected:
    /** @brief Exposes the underlying LinkSchema. For testing only. */
    inline const LinkSchema& schema() const { return this->schema_; }

   private:
    shared_ptr<AtomDB> atomdb_;
    LinkSchema schema_;
    bool read_;
    bool write_;
};

/**
 * @brief Authorization rules associated with a single public key.
 *
 * A profile is either unrestricted (full access to all atoms and operations) or
 * schema-based (access is granted when any contained AuthorizationSchema matches).
 */
class AuthorizationProfile {
   public:
    /**
     * @brief Creates a profile from unrestricted access and schema rules.
     *
     * @param full_access When true, grants unrestricted access and schemas must be empty.
     * @param schemas Schema rules used when full_access is false.
     * @throws std::runtime_error if full_access is true and schemas is not empty.
     */
    AuthorizationProfile(bool full_access, vector<shared_ptr<AuthorizationSchema>> schemas);

    ~AuthorizationProfile() = default;

    /**
     * @brief Builds a profile from a persisted access-permission document.
     *
     * @param atomdb AtomDB used by the resulting schema rules.
     * @param document Source document containing access_key, full_access flag, and entries.
     * @return Profile built from the document contents.
     */
    static shared_ptr<AuthorizationProfile> from_document(
        shared_ptr<AtomDB> atomdb,
        const shared_ptr<atomdb_api_types::AccessPermissionDocument>& document);

    /**
     * @brief Checks whether this profile grants the operation on the given atom.
     *
     * Unrestricted profiles always grant access. Otherwise, access is granted
     * when at least one schema rule matches.
     *
     * @param atom Target atom to authorize.
     * @param operation Operation being requested.
     * @return true if this profile grants the operation on the atom.
     */
    bool is_granted(shared_ptr<Atom> atom, AuthorizationOperation operation);

    /**
     * @brief Returns whether this profile grants unrestricted access.
     */
    inline bool is_unrestricted() const { return this->unrestricted_; }

   protected:
    /** @brief Exposes contained schema rules. For testing only. */
    const vector<shared_ptr<AuthorizationSchema>> schemas() const { return this->schemas_; }

   private:
    bool unrestricted_;
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
