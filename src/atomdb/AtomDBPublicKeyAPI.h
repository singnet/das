#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "AtomDBAPITypes.h"
#include "LinkSchema.h"
#include "Merger.h"
#include "Utils.h"

using namespace std;
using namespace atoms;

namespace atomdb {

/**
 * @brief Pure interface for AtomDB operations that take a PublicKey.
 *
 * Implemented by ProtectedAtomDB (local authorization) and RemoteAtomDB
 * (forwards the matching key to each protected peer).
 *
 * Callers consult get_protection_mode() on AtomDB; when the mode is PROTECTED
 * or FORWARD they use this API instead of the unkeyed AtomDB methods.
 */
class AtomDBPublicKeyAPI {
   public:
    virtual ~AtomDBPublicKeyAPI() = default;

    virtual shared_ptr<Atom> get_atom(const string& handle,
                                      const atomdb_api_types::PublicKey& public_key) = 0;
    virtual shared_ptr<Node> get_node(const string& handle,
                                      const atomdb_api_types::PublicKey& public_key) = 0;
    virtual shared_ptr<Link> get_link(const string& handle,
                                      const atomdb_api_types::PublicKey& public_key) = 0;

    virtual vector<shared_ptr<Atom>> get_matching_atoms(
        bool is_toplevel, Atom& key, const atomdb_api_types::PublicKey& public_key) = 0;

    virtual shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(
        const LinkSchema& link_schema, const atomdb_api_types::PublicKey& public_key) = 0;
    virtual shared_ptr<atomdb_api_types::HandleList> query_for_targets(
        const string& handle, const atomdb_api_types::PublicKey& public_key) = 0;
    virtual shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(
        const string& handle, const atomdb_api_types::PublicKey& public_key) = 0;

    virtual bool atom_exists(const string& handle, const atomdb_api_types::PublicKey& public_key) = 0;
    virtual bool node_exists(const string& handle, const atomdb_api_types::PublicKey& public_key) = 0;
    virtual bool link_exists(const string& handle, const atomdb_api_types::PublicKey& public_key) = 0;

    virtual set<string> atoms_exist(const vector<string>& handles,
                                    const atomdb_api_types::PublicKey& public_key) = 0;
    virtual set<string> nodes_exist(const vector<string>& handles,
                                    const atomdb_api_types::PublicKey& public_key) = 0;
    virtual set<string> links_exist(const vector<string>& handles,
                                    const atomdb_api_types::PublicKey& public_key) = 0;

    virtual string add_atom(const atoms::Atom* atom,
                            const atomdb_api_types::PublicKey& public_key,
                            const atoms::Merger* merger = NULL) = 0;
    virtual string add_node(const atoms::Node* node,
                            const atomdb_api_types::PublicKey& public_key,
                            const atoms::Merger* merger = NULL) = 0;
    virtual string add_link(const atoms::Link* link,
                            const atomdb_api_types::PublicKey& public_key,
                            const atoms::Merger* merger = NULL) = 0;

    virtual vector<string> add_atoms(const vector<atoms::Atom*>& atom_list,
                                     const atomdb_api_types::PublicKey& public_key,
                                     bool is_transactional = false,
                                     const atoms::Merger* merger = NULL) = 0;
    virtual vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                                     const atomdb_api_types::PublicKey& public_key,
                                     bool is_transactional = false,
                                     const atoms::Merger* merger = NULL) = 0;
    virtual vector<string> add_links(const vector<atoms::Link*>& links,
                                     const atomdb_api_types::PublicKey& public_key,
                                     bool is_transactional = false,
                                     const atoms::Merger* merger = NULL) = 0;

    virtual bool delete_atom(const string& handle,
                             const atomdb_api_types::PublicKey& public_key,
                             bool delete_link_targets = false) = 0;
    virtual bool delete_node(const string& handle,
                             const atomdb_api_types::PublicKey& public_key,
                             bool delete_link_targets = false) = 0;
    virtual bool delete_link(const string& handle,
                             const atomdb_api_types::PublicKey& public_key,
                             bool delete_link_targets = false) = 0;

    virtual uint delete_atoms(const vector<string>& handles,
                              const atomdb_api_types::PublicKey& public_key,
                              bool delete_link_targets = false) = 0;
    virtual uint delete_nodes(const vector<string>& handles,
                              const atomdb_api_types::PublicKey& public_key,
                              bool delete_link_targets = false) = 0;
    virtual uint delete_links(const vector<string>& handles,
                              const atomdb_api_types::PublicKey& public_key,
                              bool delete_link_targets = false) = 0;

    virtual void re_index_patterns(const atomdb_api_types::PublicKey& public_key,
                                   bool flush_patterns = true) = 0;

    virtual size_t node_count(const atomdb_api_types::PublicKey& public_key) const = 0;
    virtual size_t link_count(const atomdb_api_types::PublicKey& public_key) const = 0;
    virtual size_t atom_count(const atomdb_api_types::PublicKey& public_key) const = 0;
};

}  // namespace atomdb
