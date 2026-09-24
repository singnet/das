#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "AtomDB.h"
#include "AuthorizationManifest.h"
#include "KeySensitiveAtomDB.h"
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
class ProtectedAtomDB : public AtomDB, public KeySensitiveAtomDB {
   public:
    /**
     * @param backend Shared concrete AtomDB to wrap.
     */
    explicit ProtectedAtomDB(shared_ptr<AtomDB> backend);

    bool composite_type_enabled() const override;
    atomdb_api_types::ProtectionMode get_protection_mode() const override;

    shared_ptr<Atom> get_atom(const string& handle) override;
    shared_ptr<Atom> get_atom(const string& handle, shared_ptr<Keychain> keychain) override;

    shared_ptr<Node> get_node(const string& handle) override;
    shared_ptr<Node> get_node(const string& handle, shared_ptr<Keychain> keychain) override;

    shared_ptr<Link> get_link(const string& handle) override;
    shared_ptr<Link> get_link(const string& handle, shared_ptr<Keychain> keychain) override;

    vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel, Atom& key) override;
    vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel,
                                                Atom& key,
                                                shared_ptr<Keychain> keychain) override;

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema) override;
    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema,
                                                              shared_ptr<Keychain> keychain) override;

    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle) override;
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle,
                                                               shared_ptr<Keychain> keychain) override;

    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle) override;
    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(
        const string& handle, shared_ptr<Keychain> keychain) override;

    bool atom_exists(const string& handle) override;
    bool atom_exists(const string& handle, shared_ptr<Keychain> keychain) override;

    bool node_exists(const string& handle) override;
    bool node_exists(const string& handle, shared_ptr<Keychain> keychain) override;

    bool link_exists(const string& handle) override;
    bool link_exists(const string& handle, shared_ptr<Keychain> keychain) override;

    set<string> atoms_exist(const vector<string>& handles) override;
    set<string> atoms_exist(const vector<string>& handles, shared_ptr<Keychain> keychain) override;

    set<string> nodes_exist(const vector<string>& handles) override;
    set<string> nodes_exist(const vector<string>& handles, shared_ptr<Keychain> keychain) override;

    set<string> links_exist(const vector<string>& handles) override;
    set<string> links_exist(const vector<string>& handles, shared_ptr<Keychain> keychain) override;

    string add_atom(const atoms::Atom* atom, const atoms::Merger* merger = NULL) override;
    string add_atom(const atoms::Atom* atom,
                    shared_ptr<Keychain> keychain,
                    const atoms::Merger* merger = NULL) override;

    string add_node(const atoms::Node* node, const atoms::Merger* merger = NULL) override;
    string add_node(const atoms::Node* node,
                    shared_ptr<Keychain> keychain,
                    const atoms::Merger* merger = NULL) override;

    string add_link(const atoms::Link* link, const atoms::Merger* merger = NULL) override;
    string add_link(const atoms::Link* link,
                    shared_ptr<Keychain> keychain,
                    const atoms::Merger* merger = NULL) override;

    vector<string> add_atoms(const vector<atoms::Atom*>& atom_list,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_atoms(const vector<atoms::Atom*>& atom_list,
                             shared_ptr<Keychain> keychain,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;

    vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                             shared_ptr<Keychain> keychain,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;

    vector<string> add_links(const vector<atoms::Link*>& links,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_links(const vector<atoms::Link*>& links,
                             shared_ptr<Keychain> keychain,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;

    bool delete_atom(const string& handle, bool delete_link_targets = false) override;
    bool delete_atom(const string& handle,
                     shared_ptr<Keychain> keychain,
                     bool delete_link_targets = false) override;

    bool delete_node(const string& handle, bool delete_link_targets = false) override;
    bool delete_node(const string& handle,
                     shared_ptr<Keychain> keychain,
                     bool delete_link_targets = false) override;

    bool delete_link(const string& handle, bool delete_link_targets = false) override;
    bool delete_link(const string& handle,
                     shared_ptr<Keychain> keychain,
                     bool delete_link_targets = false) override;

    uint delete_atoms(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_atoms(const vector<string>& handles,
                      shared_ptr<Keychain> keychain,
                      bool delete_link_targets = false) override;

    uint delete_nodes(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_nodes(const vector<string>& handles,
                      shared_ptr<Keychain> keychain,
                      bool delete_link_targets = false) override;

    uint delete_links(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_links(const vector<string>& handles,
                      shared_ptr<Keychain> keychain,
                      bool delete_link_targets = false) override;

    void re_index_patterns(bool flush_patterns = true) override;
    void re_index_patterns(shared_ptr<Keychain> keychain, bool flush_patterns = true) override;

    size_t node_count() const override;
    size_t node_count(shared_ptr<Keychain> keychain) const override;

    size_t link_count() const override;
    size_t link_count(shared_ptr<Keychain> keychain) const override;

    size_t atom_count() const override;
    size_t atom_count(shared_ptr<Keychain> keychain) const override;

   private:
    shared_ptr<AtomDB> backend;
    shared_ptr<AuthorizationManifest> manifest;

    [[noreturn]] static void raise_keychain_required(const string& method_name);

    /**
     * @brief Returns the caller's public key when the Keychain identifies the caller
     *        and its authorization profile is available.
     *
     * @return The caller's public key, or nullopt if the Keychain does not provide
     *         a key for this AtomDB or the corresponding profile cannot be loaded.
     */
    optional<string> try_get_public_key(const shared_ptr<Keychain>& keychain);

    /**
     * @brief Authorizes READ access to a handle and returns the caller's public key.
     *
     * @return The caller's public key if the caller is identified and has READ
     *         permission for the handle; otherwise, nullopt.
     */
    optional<string> authorize_read(const string& handle, const shared_ptr<Keychain>& keychain);

    /**
     * @brief Authorizes READ access to an atom and returns the caller's public key.
     *
     * @return The caller's public key if the caller is identified and has READ
     *         permission for the atom; otherwise, nullopt.
     */
    optional<string> authorize_read(const shared_ptr<Atom>& atom, const shared_ptr<Keychain>& keychain);

    /**
     * @brief Returns whether the caller may READ the specified handle.
     */
    inline bool can_read(const string& handle, const string& public_key) {
        if (public_key.empty()) return false;
        return this->manifest->is_granted(public_key, handle, AuthorizationOperation::READ);
    }

    /**
     * @brief Returns whether the caller may READ the specified atom.
     */
    inline bool can_read(const shared_ptr<Atom>& atom, const string& public_key) {
        if (public_key.empty() || atom == nullptr) return false;
        return this->manifest->is_granted(public_key, atom, AuthorizationOperation::READ);
    }

    /**
     * @brief Returns the handles that exist and are readable by the caller.
     *
     * @return The subset of handles that exist in the backend and that the caller
     *         is authorized to read. Returns an empty set if the caller cannot be
     *         identified or its authorization profile cannot be loaded.
     */
    set<string> check_handles(shared_ptr<Keychain> keychain, const function<set<string>()>& exists_many);
    /**
     * @brief Filters a backend handle set to include only handles readable by the caller.
     *
     * @return A handle set containing only authorized handles. Returns an empty
     *         handle set if the caller is not authorized.
     */
    shared_ptr<atomdb_api_types::HandleSet> filter_handle_set(
        const optional<string>& public_key,
        const function<shared_ptr<atomdb_api_types::HandleSet>()>& query);

    /**
     * @brief Filters a set of handles to include only those readable by the caller.
     *
     * @return The subset of original_handles that the caller is authorized to read.
     */
    set<string> filter_handles(const set<string>& original_handles, const string& public_key);

    /**
     * @brief Filters a collection of atoms to include only those readable by the caller.
     *
     * @return The subset of original_atoms that the caller is authorized to read.
     */
    vector<shared_ptr<Atom>> filter_atoms(const vector<shared_ptr<Atom>>& original_atoms,
                                          const string& public_key);
};

}  // namespace atomdb
