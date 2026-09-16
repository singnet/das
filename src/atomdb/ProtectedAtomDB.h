#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "AtomDB.h"
#include "AtomDBKeySensitive.h"
#include "AuthorizationManifest.h"
#include "Keychain.h"

using namespace std;
using namespace atoms;

namespace atomdb {

/**
 * @brief Authorization wrapper around any AtomDB backend for protected databases.
 *
 * Data-access methods expose two forms:
 * - overloads without Keychain: reject the call because protected access requires authentication.
 * - overloads with Keychain: authorize the request and delegate to the backend.
 *
 * This class implements ProtectionMode::PROTECTED by filtering reads and
 * queries according to the permissions associated with the caller's Keychain.
 */
class ProtectedAtomDB : public AtomDB, public AtomDBKeySensitive {
   public:
    /**
     * @param backend Shared concrete AtomDB to wrap.
     */
    explicit ProtectedAtomDB(shared_ptr<AtomDB> backend);

    bool composite_type_enabled() const override;
    atomdb_api_types::ProtectionMode get_protection_mode() const override;

    shared_ptr<Atom> get_atom(const string& handle) override;
    shared_ptr<Atom> get_atom(const string& handle, shared_ptr<Keychain> keychain);

    shared_ptr<Node> get_node(const string& handle) override;
    shared_ptr<Node> get_node(const string& handle, shared_ptr<Keychain> keychain);

    shared_ptr<Link> get_link(const string& handle) override;
    shared_ptr<Link> get_link(const string& handle, shared_ptr<Keychain> keychain);

    vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel, Atom& key) override;
    vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel,
                                                Atom& key,
                                                shared_ptr<Keychain> keychain);

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema) override;
    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema,
                                                              shared_ptr<Keychain> keychain);

    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle) override;
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle,
                                                               shared_ptr<Keychain> keychain);

    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle) override;
    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle,
                                                                   shared_ptr<Keychain> keychain);

    bool atom_exists(const string& handle) override;
    bool atom_exists(const string& handle, shared_ptr<Keychain> keychain);

    bool node_exists(const string& handle) override;
    bool node_exists(const string& handle, shared_ptr<Keychain> keychain);

    bool link_exists(const string& handle) override;
    bool link_exists(const string& handle, shared_ptr<Keychain> keychain);

    set<string> atoms_exist(const vector<string>& handles) override;
    set<string> atoms_exist(const vector<string>& handles, shared_ptr<Keychain> keychain);

    set<string> nodes_exist(const vector<string>& handles) override;
    set<string> nodes_exist(const vector<string>& handles, shared_ptr<Keychain> keychain);

    set<string> links_exist(const vector<string>& handles) override;
    set<string> links_exist(const vector<string>& handles, shared_ptr<Keychain> keychain);

    string add_atom(const atoms::Atom* atom, const atoms::Merger* merger = NULL) override;
    string add_atom(const atoms::Atom* atom,
                    shared_ptr<Keychain> keychain,
                    const atoms::Merger* merger = NULL);

    string add_node(const atoms::Node* node, const atoms::Merger* merger = NULL) override;
    string add_node(const atoms::Node* node,
                    shared_ptr<Keychain> keychain,
                    const atoms::Merger* merger = NULL);

    string add_link(const atoms::Link* link, const atoms::Merger* merger = NULL) override;
    string add_link(const atoms::Link* link,
                    shared_ptr<Keychain> keychain,
                    const atoms::Merger* merger = NULL);

    vector<string> add_atoms(const vector<atoms::Atom*>& atom_list,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_atoms(const vector<atoms::Atom*>& atom_list,
                             shared_ptr<Keychain> keychain,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL);

    vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                             shared_ptr<Keychain> keychain,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL);

    vector<string> add_links(const vector<atoms::Link*>& links,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_links(const vector<atoms::Link*>& links,
                             shared_ptr<Keychain> keychain,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL);

    bool delete_atom(const string& handle, bool delete_link_targets = false) override;
    bool delete_atom(const string& handle,
                     shared_ptr<Keychain> keychain,
                     bool delete_link_targets = false);

    bool delete_node(const string& handle, bool delete_link_targets = false) override;
    bool delete_node(const string& handle,
                     shared_ptr<Keychain> keychain,
                     bool delete_link_targets = false);

    bool delete_link(const string& handle, bool delete_link_targets = false) override;
    bool delete_link(const string& handle,
                     shared_ptr<Keychain> keychain,
                     bool delete_link_targets = false);

    uint delete_atoms(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_atoms(const vector<string>& handles,
                      shared_ptr<Keychain> keychain,
                      bool delete_link_targets = false);

    uint delete_nodes(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_nodes(const vector<string>& handles,
                      shared_ptr<Keychain> keychain,
                      bool delete_link_targets = false);

    uint delete_links(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_links(const vector<string>& handles,
                      shared_ptr<Keychain> keychain,
                      bool delete_link_targets = false);

    void re_index_patterns(bool flush_patterns = true) override;
    void re_index_patterns(shared_ptr<Keychain> keychain, bool flush_patterns = true);

    size_t node_count() const override;
    size_t node_count(shared_ptr<Keychain> keychain) const;

    size_t link_count() const override;
    size_t link_count(shared_ptr<Keychain> keychain) const;

    size_t atom_count() const override;
    size_t atom_count(shared_ptr<Keychain> keychain) const;

   private:
    shared_ptr<AtomDB> backend;
    shared_ptr<AuthorizationManifest> manifest;

    [[noreturn]] static void raise_public_key_required(const string& method_name);

    /**
     * @brief Whether public_key may READ the atom identified by handle.
     *
     * This overload resolves the atom from the backend before checking
     * permissions. Prefer the Atom overload when the atom is already
     * available.
     */
    bool can_read(const string& public_key, const string& handle);

    /**
     * @brief Whether public_key may READ atom.
     *
     * Avoids an additional backend lookup when the atom is already available.
     *
     * @return false if atom is null or the associated profile denies READ.
     */
    bool can_read(const string& public_key, const shared_ptr<Atom>& atom);

    /**
     * @brief Ensures that public_key is loaded into the authorization manifest.
     *
     * If the key is not already cached, its access-permission document is
     * loaded from the backend and registered in the manifest.
     *
     * @return true if public_key is registered after the lookup attempt.
     */
    bool ensure_registered(const string& public_key);

    /**
     * @brief Returns a filtered copy of original_handle_set containing only handles that public_key may
     * READ.
     */
    shared_ptr<atomdb_api_types::HandleSet> filter_handle_set(
        const shared_ptr<atomdb_api_types::HandleSet>& original_handle_set, const string& public_key);

    /**
     * @brief Returns the subset of original_handles that public_key may READ.
     */
    set<string> filter_handles(const set<string>& original_handles, const string& public_key);
};

}  // namespace atomdb
