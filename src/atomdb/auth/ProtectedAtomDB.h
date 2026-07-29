#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "AtomDB.h"
#include "JsonConfig.h"

using namespace std;
using namespace commons;
using namespace atoms;

namespace atomdb {

/**
 * @brief Authorization wrapper around any AtomDB backend for protected databases.
 *
 * Delegates storage to backend. When public_key is non-empty, builds AuthorizationManagement for that
 * key and filters read results. When public_key is empty, delegates without filtering
 * filtering happens in this wrapper after the backend returns data.
 */
class ProtectedAtomDB : public AtomDB {
   public:
    /**
     * @param backend Shared concrete AtomDB.
     * @param config Settings used to construct AuthorizationManagement per key.
     */
    ProtectedAtomDB(shared_ptr<AtomDB> backend, const JsonConfig& config);

    bool allow_nested_indexing(const string& public_key) override;
    bool composite_type_enabled() const override;
    bool is_protected() const override;

    shared_ptr<Atom> get_atom(const string& handle, const string& public_key) override;
    shared_ptr<Node> get_node(const string& handle, const string& public_key) override;
    shared_ptr<Link> get_link(const string& handle, const string& public_key) override;

    vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel,
                                                Atom& key,
                                                const string& public_key) override;

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema,
                                                              const string& public_key) override;
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle,
                                                               const string& public_key) override;
    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle,
                                                                   const string& public_key) override;

    bool atom_exists(const string& handle, const string& public_key) override;
    bool node_exists(const string& handle, const string& public_key) override;
    bool link_exists(const string& handle, const string& public_key) override;

    set<string> atoms_exist(const vector<string>& handles, const string& public_key) override;
    set<string> nodes_exist(const vector<string>& handles, const string& public_key) override;
    set<string> links_exist(const vector<string>& handles, const string& public_key) override;

    string add_atom(const atoms::Atom* atom,
                    const string& public_key,
                    bool throw_if_exists = false) override;
    string add_node(const atoms::Node* node,
                    const string& public_key,
                    bool throw_if_exists = false) override;
    string add_link(const atoms::Link* link,
                    const string& public_key,
                    bool throw_if_exists = false) override;

    vector<string> add_atoms(const vector<atoms::Atom*>& atoms,
                             const string& public_key,
                             bool throw_if_exists = false,
                             bool is_transactional = false) override;
    vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                             const string& public_key,
                             bool throw_if_exists = false,
                             bool is_transactional = false) override;
    vector<string> add_links(const vector<atoms::Link*>& links,
                             const string& public_key,
                             bool throw_if_exists = false,
                             bool is_transactional = false) override;

    bool delete_atom(const string& handle,
                     const string& public_key,
                     bool delete_link_targets = false) override;
    bool delete_node(const string& handle,
                     const string& public_key,
                     bool delete_link_targets = false) override;
    bool delete_link(const string& handle,
                     const string& public_key,
                     bool delete_link_targets = false) override;

    uint delete_atoms(const vector<string>& handles,
                      const string& public_key,
                      bool delete_link_targets = false) override;
    uint delete_nodes(const vector<string>& handles,
                      const string& public_key,
                      bool delete_link_targets = false) override;
    uint delete_links(const vector<string>& handles,
                      const string& public_key,
                      bool delete_link_targets = false) override;

    void re_index_patterns(const string& public_key, bool flush_patterns = true) override;

    size_t node_count(const string& public_key) const override;
    size_t link_count(const string& public_key) const override;
    size_t atom_count(const string& public_key) const override;

   private:
    shared_ptr<AtomDB> backend;
    JsonConfig config;

    /**
     * @brief Read check for public_key and handle on this wrapper.
     *
     * @param public_key key from the AtomDB API. Empty means allow (no filter).
     * @param handle Handle to check.
     * @return true if key is empty or AccessControl for that key allows read.
     */
    bool can_read(const string& public_key, const string& handle);

    /**
     * @brief Read check for public_key and atom on this wrapper.
     *
     * @param public_key key from the AtomDB API.
     * @param atom Atom to check.
     * @return true if key is empty or AccessControl for that key allows read.
     */
    bool can_read(const string& public_key, const atoms::Atom& atom);

    /**
     * @brief Builds a filtered HandleSet from a backend result.
     *
     * @param raw Unfiltered result from backend.
     * @param public_key key used for each handle check.
     * @return HandleSet containing only authorized handles.
     */
    shared_ptr<atomdb_api_types::HandleSet> filter_handle_set(
        shared_ptr<atomdb_api_types::HandleSet> raw, const string& public_key);

    /**
     * @brief Builds a filtered HandleList from a backend result.
     *
     * @param raw Unfiltered result from backend.
     * @param public_key key used for each handle check.
     * @return HandleList containing only authorized handles.
     */
    shared_ptr<atomdb_api_types::HandleList> filter_handle_list(
        shared_ptr<atomdb_api_types::HandleList> raw, const string& public_key);
};

}  // namespace atomdb
