#pragma once

#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "Atom.h"
#include "AtomDBAPITypes.h"
#include "HandleDecoder.h"
#include "Keychain.h"
#include "Link.h"
#include "LinkSchema.h"
#include "Merger.h"
#include "Utils.h"

using namespace std;
using namespace atoms;

namespace atomdb {

/**
 * @brief AtomDB operations that take a Keychain.
 */
class AtomDBKeySensitive {
   public:
    virtual ~AtomDBKeySensitive() = default;

    virtual shared_ptr<Atom> get_atom(const string& handle, shared_ptr<Keychain> keychain) = 0;
    virtual shared_ptr<Node> get_node(const string& handle, shared_ptr<Keychain> keychain) = 0;
    virtual shared_ptr<Link> get_link(const string& handle, shared_ptr<Keychain> keychain) = 0;

    virtual vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel,
                                                        Atom& key,
                                                        shared_ptr<Keychain> keychain) = 0;

    virtual shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema,
                                                                      shared_ptr<Keychain> keychain) = 0;
    virtual shared_ptr<atomdb_api_types::HandleList> query_for_targets(
        const string& handle, shared_ptr<Keychain> keychain) = 0;
    virtual shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(
        const string& handle, shared_ptr<Keychain> keychain) = 0;

    virtual bool atom_exists(const string& handle, shared_ptr<Keychain> keychain) = 0;
    virtual bool node_exists(const string& handle, shared_ptr<Keychain> keychain) = 0;
    virtual bool link_exists(const string& handle, shared_ptr<Keychain> keychain) = 0;

    virtual set<string> atoms_exist(const vector<string>& handles, shared_ptr<Keychain> keychain) = 0;
    virtual set<string> nodes_exist(const vector<string>& handles, shared_ptr<Keychain> keychain) = 0;
    virtual set<string> links_exist(const vector<string>& handles, shared_ptr<Keychain> keychain) = 0;

    virtual string add_atom(const atoms::Atom* atom,
                            shared_ptr<Keychain> keychain,
                            const atoms::Merger* merger = NULL) = 0;
    virtual string add_node(const atoms::Node* node,
                            shared_ptr<Keychain> keychain,
                            const atoms::Merger* merger = NULL) = 0;
    virtual string add_link(const atoms::Link* link,
                            shared_ptr<Keychain> keychain,
                            const atoms::Merger* merger = NULL) = 0;

    virtual vector<string> add_atoms(const vector<atoms::Atom*>& atom_list,
                                     shared_ptr<Keychain> keychain,
                                     bool is_transactional = false,
                                     const atoms::Merger* merger = NULL) = 0;
    virtual vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                                     shared_ptr<Keychain> keychain,
                                     bool is_transactional = false,
                                     const atoms::Merger* merger = NULL) = 0;
    virtual vector<string> add_links(const vector<atoms::Link*>& links,
                                     shared_ptr<Keychain> keychain,
                                     bool is_transactional = false,
                                     const atoms::Merger* merger = NULL) = 0;

    virtual bool delete_atom(const string& handle,
                             shared_ptr<Keychain> keychain,
                             bool delete_link_targets = false) = 0;
    virtual bool delete_node(const string& handle,
                             shared_ptr<Keychain> keychain,
                             bool delete_link_targets = false) = 0;
    virtual bool delete_link(const string& handle,
                             shared_ptr<Keychain> keychain,
                             bool delete_link_targets = false) = 0;

    virtual uint delete_atoms(const vector<string>& handles,
                              shared_ptr<Keychain> keychain,
                              bool delete_link_targets = false) = 0;
    virtual uint delete_nodes(const vector<string>& handles,
                              shared_ptr<Keychain> keychain,
                              bool delete_link_targets = false) = 0;
    virtual uint delete_links(const vector<string>& handles,
                              shared_ptr<Keychain> keychain,
                              bool delete_link_targets = false) = 0;

    virtual void re_index_patterns(shared_ptr<Keychain> keychain, bool flush_patterns = true) = 0;

    virtual size_t node_count(shared_ptr<Keychain> keychain) const = 0;
    virtual size_t link_count(shared_ptr<Keychain> keychain) const = 0;
    virtual size_t atom_count(shared_ptr<Keychain> keychain) const = 0;
};

}  // namespace atomdb
