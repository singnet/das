#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "AtomDB.h"
#include "AtomDBPublicKeyAPI.h"
#include "AuthorizationManifest.h"

using namespace std;
using namespace atoms;

namespace atomdb {

/**
 * @brief Authorization wrapper around a protected AtomDB backend.
 *
 * Implements AtomDB (unkeyed methods reject the call) and AtomDBPublicKeyAPI
 * (authorize, then delegate to the backend).
 *
 * get_protection_mode() reports the backend's mode so callers can detect
 * protected persistence without inspecting the wrapper type.
 */
class ProtectedAtomDB : public AtomDB, public AtomDBPublicKeyAPI {
   public:
    /**
     * @param backend Shared concrete AtomDB to wrap.
     */
    explicit ProtectedAtomDB(shared_ptr<AtomDB> backend);

    bool allow_nested_indexing() override;
    bool composite_type_enabled() const override;
    atomdb_api_types::ProtectionMode get_protection_mode() const override;

    shared_ptr<Atom> get_atom(const string& handle) override;
    shared_ptr<Atom> get_atom(const string& handle,
                              const atomdb_api_types::PublicKey& public_key) override;

    shared_ptr<Node> get_node(const string& handle) override;
    shared_ptr<Node> get_node(const string& handle,
                              const atomdb_api_types::PublicKey& public_key) override;

    shared_ptr<Link> get_link(const string& handle) override;
    shared_ptr<Link> get_link(const string& handle,
                              const atomdb_api_types::PublicKey& public_key) override;

    vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel, Atom& key) override;
    vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel,
                                                Atom& key,
                                                const atomdb_api_types::PublicKey& public_key) override;

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema) override;
    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(
        const LinkSchema& link_schema, const atomdb_api_types::PublicKey& public_key) override;

    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle) override;
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(
        const string& handle, const atomdb_api_types::PublicKey& public_key) override;

    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle) override;
    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(
        const string& handle, const atomdb_api_types::PublicKey& public_key) override;

    bool atom_exists(const string& handle) override;
    bool atom_exists(const string& handle, const atomdb_api_types::PublicKey& public_key) override;

    bool node_exists(const string& handle) override;
    bool node_exists(const string& handle, const atomdb_api_types::PublicKey& public_key) override;

    bool link_exists(const string& handle) override;
    bool link_exists(const string& handle, const atomdb_api_types::PublicKey& public_key) override;

    set<string> atoms_exist(const vector<string>& handles) override;
    set<string> atoms_exist(const vector<string>& handles,
                            const atomdb_api_types::PublicKey& public_key) override;

    set<string> nodes_exist(const vector<string>& handles) override;
    set<string> nodes_exist(const vector<string>& handles,
                            const atomdb_api_types::PublicKey& public_key) override;

    set<string> links_exist(const vector<string>& handles) override;
    set<string> links_exist(const vector<string>& handles,
                            const atomdb_api_types::PublicKey& public_key) override;

    string add_atom(const atoms::Atom* atom, const atoms::Merger* merger = NULL) override;
    string add_atom(const atoms::Atom* atom,
                    const atomdb_api_types::PublicKey& public_key,
                    const atoms::Merger* merger = NULL) override;

    string add_node(const atoms::Node* node, const atoms::Merger* merger = NULL) override;
    string add_node(const atoms::Node* node,
                    const atomdb_api_types::PublicKey& public_key,
                    const atoms::Merger* merger = NULL) override;

    string add_link(const atoms::Link* link, const atoms::Merger* merger = NULL) override;
    string add_link(const atoms::Link* link,
                    const atomdb_api_types::PublicKey& public_key,
                    const atoms::Merger* merger = NULL) override;

    vector<string> add_atoms(const vector<atoms::Atom*>& atom_list,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_atoms(const vector<atoms::Atom*>& atom_list,
                             const atomdb_api_types::PublicKey& public_key,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;

    vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                             const atomdb_api_types::PublicKey& public_key,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;

    vector<string> add_links(const vector<atoms::Link*>& links,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_links(const vector<atoms::Link*>& links,
                             const atomdb_api_types::PublicKey& public_key,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;

    bool delete_atom(const string& handle, bool delete_link_targets = false) override;
    bool delete_atom(const string& handle,
                     const atomdb_api_types::PublicKey& public_key,
                     bool delete_link_targets = false) override;

    bool delete_node(const string& handle, bool delete_link_targets = false) override;
    bool delete_node(const string& handle,
                     const atomdb_api_types::PublicKey& public_key,
                     bool delete_link_targets = false) override;

    bool delete_link(const string& handle, bool delete_link_targets = false) override;
    bool delete_link(const string& handle,
                     const atomdb_api_types::PublicKey& public_key,
                     bool delete_link_targets = false) override;

    uint delete_atoms(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_atoms(const vector<string>& handles,
                      const atomdb_api_types::PublicKey& public_key,
                      bool delete_link_targets = false) override;

    uint delete_nodes(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_nodes(const vector<string>& handles,
                      const atomdb_api_types::PublicKey& public_key,
                      bool delete_link_targets = false) override;

    uint delete_links(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_links(const vector<string>& handles,
                      const atomdb_api_types::PublicKey& public_key,
                      bool delete_link_targets = false) override;

    void re_index_patterns(bool flush_patterns = true) override;
    void re_index_patterns(const atomdb_api_types::PublicKey& public_key,
                           bool flush_patterns = true) override;

    size_t node_count() const override;
    size_t node_count(const atomdb_api_types::PublicKey& public_key) const override;

    size_t link_count() const override;
    size_t link_count(const atomdb_api_types::PublicKey& public_key) const override;

    size_t atom_count() const override;
    size_t atom_count(const atomdb_api_types::PublicKey& public_key) const override;

   private:
    shared_ptr<AtomDB> backend;
    shared_ptr<AuthorizationManifest> manifest;

    [[noreturn]] static void raise_public_key_required(const string& method_name);

    /**
     * @brief Loads access-permission documents for public_key into the manifest when missing.
     *
     * @return true if the key is registered after the lookup attempt.
     */
    bool ensure_registered(const atomdb_api_types::PublicKey& public_key);

    bool can_read(const atomdb_api_types::PublicKey& public_key, const string& handle);
    bool can_read(const atomdb_api_types::PublicKey& public_key, const atoms::Atom& atom);
    bool can_write(const atomdb_api_types::PublicKey& public_key, const atoms::Atom& atom);
    bool can_write(const atomdb_api_types::PublicKey& public_key, const string& handle);

    shared_ptr<atomdb_api_types::HandleSet> filter_handle_set(
        shared_ptr<atomdb_api_types::HandleSet> raw, const atomdb_api_types::PublicKey& public_key);
    shared_ptr<atomdb_api_types::HandleList> filter_handle_list(
        shared_ptr<atomdb_api_types::HandleList> raw, const atomdb_api_types::PublicKey& public_key);
};

}  // namespace atomdb
