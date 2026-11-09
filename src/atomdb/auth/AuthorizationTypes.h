#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "AtomDB.h"
#include "AtomDBAPITypes.h"
#include "HandleDecoder.h"
#include "Keychain.h"
#include "LinkSchema.h"
#include "atomdb/AtomDBUtils.h"

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

    /**
     * @brief Adds the atoms reached by this schema to the three counters.
     *
     * @param node_count The number of nodes matched by the schema.
     * @param link_count The number of links matched by the schema.
     * @param atom_count The number of atoms matched by the schema.
     * @param keychain The keychain to count atoms for.
     */
    void count_matching_atoms(size_t& node_count,
                              size_t& link_count,
                              size_t& atom_count,
                              shared_ptr<Keychain> keychain,
                              set<string>& visited);

    // These method are used only for testing purposes.
   protected:
    inline const LinkSchema& schema() const { return this->schema_; }

   private:
    class AuthorizationSchemaDecoder : public HandleDecoder {
       public:
        shared_ptr<AtomDB> atomdb;
        AuthorizationSchemaDecoder(shared_ptr<AtomDB> atomdb) : atomdb(atomdb) {}
        shared_ptr<Atom> decode_atom(const string& handle,
                                     shared_ptr<Keychain> keychain = nullptr) override {
            return this->atomdb->get_atom(handle);
        }
        string decode_node_name(const string& handle, shared_ptr<Keychain> keychain = nullptr) override {
            auto node = this->atomdb->get_node(handle);
            if (node == nullptr) {
                return "";
            }
            return node->name;
        }
        vector<string> decode_link_targets(const string& handle,
                                           shared_ptr<Keychain> keychain = nullptr) override {
            auto link = this->atomdb->get_link(handle);
            if (link == nullptr) {
                return vector<string>();
            }
            return link->targets;
        }
    };

    shared_ptr<AtomDB> atomdb_;
    shared_ptr<AuthorizationSchemaDecoder> decoder_;
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

    /**
     * @brief Adds the atoms granted by this profile's read schemas to the three counters.
     */
    void count_matching_atoms(size_t& node_count,
                              size_t& link_count,
                              size_t& atom_count,
                              shared_ptr<Keychain> keychain);

    // These method are used only for testing purposes.
   protected:
    const vector<shared_ptr<AuthorizationSchema>> schemas() const { return this->schemas_; }

   private:
    bool full_access_;
    vector<shared_ptr<AuthorizationSchema>> schemas_;
};

}  // namespace atomdb
