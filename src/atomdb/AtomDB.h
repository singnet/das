#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "AtomDBAPITypes.h"
#include "HandleDecoder.h"
#include "LinkSchema.h"
#include "Merger.h"
#include "Properties.h"

using namespace std;
using namespace commons;
using namespace atoms;

namespace atomdb {

enum class AtomDBType { RedisMongoDB, MorkDB, InMemoryDB, RemoteAtomDB, AdapterDB };

class AtomDB : public HandleDecoder {
   public:
    AtomDB() = default;
    virtual ~AtomDB() = default;

    static AtomDBType string_to_type(const string& type) {
        if (type == "redismongodb") return AtomDBType::RedisMongoDB;
        if (type == "morkdb") return AtomDBType::MorkDB;
        if (type == "inmemorydb") return AtomDBType::InMemoryDB;
        if (type == "remotedb") return AtomDBType::RemoteAtomDB;
        if (type == "adapterdb") return AtomDBType::AdapterDB;
        RAISE_ERROR("Unsupported atomdb.type: " + type);
    }

    static string type_to_string(AtomDBType type) {
        switch (type) {
            case AtomDBType::RedisMongoDB:
                return "redismongodb";
            case AtomDBType::MorkDB:
                return "morkdb";
            case AtomDBType::InMemoryDB:
                return "inmemorydb";
            case AtomDBType::RemoteAtomDB:
                return "remotedb";
            case AtomDBType::AdapterDB:
                return "adapterdb";
            default:
                RAISE_ERROR("Unsupported AtomDBType");
        }
    }

    virtual bool allow_nested_indexing() = 0;
    virtual bool composite_type_enabled() const = 0;
    virtual atomdb_api_types::ProtectionMode get_protection_mode() const = 0;

    virtual shared_ptr<Atom> get_atom(const string& handle) = 0;  // HandleDecoder interface
    virtual shared_ptr<Node> get_node(const string& handle) = 0;
    virtual shared_ptr<Link> get_link(const string& handle) = 0;

    virtual vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel, Atom& key) = 0;

    virtual shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema) = 0;
    virtual shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle) = 0;
    virtual shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle) = 0;

    virtual bool atom_exists(const string& handle) = 0;
    virtual bool node_exists(const string& handle) = 0;
    virtual bool link_exists(const string& handle) = 0;

    virtual set<string> atoms_exist(const vector<string>& handles) = 0;
    virtual set<string> nodes_exist(const vector<string>& handles) = 0;
    virtual set<string> links_exist(const vector<string>& handles) = 0;

    /**
     * Add methods take an optional Merger.
     * - merger == NULL: upsert (insert if missing, replace if present)
     * - merger != NULL and atom exists: merge into a working copy, then persist only if
     *   merge() returns true; if merge() returns false, stored state is left unchanged
     *   and the returned handle (or batch slot) is ""
     * - Use &ThrowIfExistsMerger::instance() to reject duplicates by throwing
     *   (former throw_if_exists=true); earlier items in a batch may already be applied
     * - Use &SkipIfExistsMerger::instance() to soft-skip duplicates (merge returns false,
     *   returned handle/slot is "", batch continues)
     * - Soft merge() returns false does not abort the rest of a batch
     * - Merge-enabled adds assume a single writer per handle (no concurrent RMW atomicity)
     */
    virtual string add_atom(const atoms::Atom* atom, const atoms::Merger* merger = NULL) = 0;
    virtual string add_node(const atoms::Node* node, const atoms::Merger* merger = NULL) = 0;
    virtual string add_link(const atoms::Link* link, const atoms::Merger* merger = NULL) = 0;

    virtual vector<string> add_atoms(const vector<atoms::Atom*>& atom_list,
                                     bool is_transactional = false,
                                     const atoms::Merger* merger = NULL) = 0;
    virtual vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                                     bool is_transactional = false,
                                     const atoms::Merger* merger = NULL) = 0;
    virtual vector<string> add_links(const vector<atoms::Link*>& links,
                                     bool is_transactional = false,
                                     const atoms::Merger* merger = NULL) = 0;

    virtual bool delete_atom(const string& handle, bool delete_link_targets = false) = 0;
    virtual bool delete_node(const string& handle, bool delete_link_targets = false) = 0;
    virtual bool delete_link(const string& handle, bool delete_link_targets = false) = 0;

    virtual uint delete_atoms(const vector<string>& handles, bool delete_link_targets = false) = 0;
    virtual uint delete_nodes(const vector<string>& handles, bool delete_link_targets = false) = 0;
    virtual uint delete_links(const vector<string>& handles, bool delete_link_targets = false) = 0;

    virtual void re_index_patterns(bool flush_patterns = true) = 0;

    virtual size_t node_count() const = 0;
    virtual size_t link_count() const = 0;
    virtual size_t atom_count() const = 0;

    bool empty() const { return atom_count() == 0; }

    virtual vector<atomdb_api_types::AccessPermissionDocument> get_access_permissions(
        const atomdb_api_types::PublicKey& public_key) const {
        return {};
    }
};

}  // namespace atomdb
