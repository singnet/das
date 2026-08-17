#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "AtomDB.h"

using namespace std;
using namespace atoms;

namespace atomdb {

/**
 * @brief Authorization wrapper around any AtomDB backend for protected databases.
 *
 * Data-access methods expose two forms:
 * - overloads without PublicKey: reject the call (protected access requires a key)
 * - overloads with PublicKey: authorize and delegate to the backend
 *
 * When the backend reports ProtectionMode::FORWARD, this wrapper forwards the
 * access key without applying local authorization post-processing.
 */
class ProtectedAtomDB : public AtomDB {
   public:
    /**
     * @param backend Shared concrete AtomDB to wrap.
     */
    explicit ProtectedAtomDB(shared_ptr<AtomDB> backend);

    bool allow_nested_indexing() override;
    bool composite_type_enabled() const override;
    atomdb_api_types::ProtectionMode is_protected() const override;
    vector<atomdb_api_types::AccessPermissionDocument> get_access_permissions(
        const atomdb_api_types::PublicKey& public_key) const override;

    shared_ptr<Atom> get_atom(const string& handle) override;
    shared_ptr<Atom> get_atom(const string& handle, const atomdb_api_types::PublicKey& public_key);

    shared_ptr<Node> get_node(const string& handle) override;
    shared_ptr<Node> get_node(const string& handle, const atomdb_api_types::PublicKey& public_key);

    shared_ptr<Link> get_link(const string& handle) override;
    shared_ptr<Link> get_link(const string& handle, const atomdb_api_types::PublicKey& public_key);

    vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel, Atom& key) override;
    vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel,
                                                Atom& key,
                                                const atomdb_api_types::PublicKey& public_key);

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema) override;
    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema,
                                                              const atomdb_api_types::PublicKey& public_key);

    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle) override;
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle,
                                                               const atomdb_api_types::PublicKey& public_key);

    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle) override;
    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle,
                                                                   const atomdb_api_types::PublicKey& public_key);

    bool atom_exists(const string& handle) override;
    bool atom_exists(const string& handle, const atomdb_api_types::PublicKey& public_key);

    bool node_exists(const string& handle) override;
    bool node_exists(const string& handle, const atomdb_api_types::PublicKey& public_key);

    bool link_exists(const string& handle) override;
    bool link_exists(const string& handle, const atomdb_api_types::PublicKey& public_key);

    set<string> atoms_exist(const vector<string>& handles) override;
    set<string> atoms_exist(const vector<string>& handles, const atomdb_api_types::PublicKey& public_key);

    set<string> nodes_exist(const vector<string>& handles) override;
    set<string> nodes_exist(const vector<string>& handles, const atomdb_api_types::PublicKey& public_key);

    set<string> links_exist(const vector<string>& handles) override;
    set<string> links_exist(const vector<string>& handles, const atomdb_api_types::PublicKey& public_key);

    string add_atom(const atoms::Atom* atom, const atoms::Merger* merger = NULL) override;
    string add_atom(const atoms::Atom* atom,
                    const atomdb_api_types::PublicKey& public_key,
                    const atoms::Merger* merger = NULL);

    string add_node(const atoms::Node* node, const atoms::Merger* merger = NULL) override;
    string add_node(const atoms::Node* node,
                    const atomdb_api_types::PublicKey& public_key,
                    const atoms::Merger* merger = NULL);

    string add_link(const atoms::Link* link, const atoms::Merger* merger = NULL) override;
    string add_link(const atoms::Link* link,
                    const atomdb_api_types::PublicKey& public_key,
                    const atoms::Merger* merger = NULL);

    vector<string> add_atoms(const vector<atoms::Atom*>& atom_list,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_atoms(const vector<atoms::Atom*>& atom_list,
                             const atomdb_api_types::PublicKey& public_key,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL);

    vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                             const atomdb_api_types::PublicKey& public_key,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL);

    vector<string> add_links(const vector<atoms::Link*>& links,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_links(const vector<atoms::Link*>& links,
                             const atomdb_api_types::PublicKey& public_key,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL);

    bool delete_atom(const string& handle, bool delete_link_targets = false) override;
    bool delete_atom(const string& handle,
                     const atomdb_api_types::PublicKey& public_key,
                     bool delete_link_targets = false);

    bool delete_node(const string& handle, bool delete_link_targets = false) override;
    bool delete_node(const string& handle,
                     const atomdb_api_types::PublicKey& public_key,
                     bool delete_link_targets = false);

    bool delete_link(const string& handle, bool delete_link_targets = false) override;
    bool delete_link(const string& handle,
                     const atomdb_api_types::PublicKey& public_key,
                     bool delete_link_targets = false);

    uint delete_atoms(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_atoms(const vector<string>& handles,
                      const atomdb_api_types::PublicKey& public_key,
                      bool delete_link_targets = false);

    uint delete_nodes(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_nodes(const vector<string>& handles,
                      const atomdb_api_types::PublicKey& public_key,
                      bool delete_link_targets = false);

    uint delete_links(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_links(const vector<string>& handles,
                      const atomdb_api_types::PublicKey& public_key,
                      bool delete_link_targets = false);

    void re_index_patterns(bool flush_patterns = true) override;
    void re_index_patterns(const atomdb_api_types::PublicKey& public_key, bool flush_patterns = true);

    size_t node_count() const override;
    size_t node_count(const atomdb_api_types::PublicKey& public_key) const;

    size_t link_count() const override;
    size_t link_count(const atomdb_api_types::PublicKey& public_key) const;

    size_t atom_count() const override;
    size_t atom_count(const atomdb_api_types::PublicKey& public_key) const;

   private:
    shared_ptr<AtomDB> backend;

    [[noreturn]] static void raise_public_key_required(const string& method_name);
};

}  // namespace atomdb
