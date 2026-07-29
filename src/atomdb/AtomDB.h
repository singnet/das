#pragma once

#include <set>
#include <string>
#include <vector>

#include "AtomDBAPITypes.h"
#include "HandleDecoder.h"
#include "LinkSchema.h"
#include "Properties.h"

using namespace std;
using namespace commons;
using namespace atoms;

namespace atomdb {

class AtomDB : public HandleDecoder {
   public:
    AtomDB() = default;
    virtual ~AtomDB() = default;

    virtual bool allow_nested_indexing(const string& public_key) = 0;
    virtual bool composite_type_enabled() const = 0;

    /**
     * @brief Reports whether this backend points to a protected database.
     */
    virtual bool is_protected() const = 0;

    /**
     * HandleDecoder requires get_atom(handle) without public_key. Existing callers use that interface,
     * so this forwards to get_atom(handle, "").
     */
    shared_ptr<Atom> get_atom(const string& handle) override { return get_atom(handle, ""); }

    virtual shared_ptr<Atom> get_atom(const string& handle, const string& public_key) = 0;
    virtual shared_ptr<Node> get_node(const string& handle, const string& public_key) = 0;
    virtual shared_ptr<Link> get_link(const string& handle, const string& public_key) = 0;

    virtual vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel,
                                                        Atom& key,
                                                        const string& public_key) = 0;

    virtual shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema,
                                                                      const string& public_key) = 0;
    virtual shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle,
                                                                       const string& public_key) = 0;
    virtual shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle,
                                                                           const string& public_key) = 0;

    virtual bool atom_exists(const string& handle, const string& public_key) = 0;
    virtual bool node_exists(const string& handle, const string& public_key) = 0;
    virtual bool link_exists(const string& handle, const string& public_key) = 0;

    virtual set<string> atoms_exist(const vector<string>& handles, const string& public_key) = 0;
    virtual set<string> nodes_exist(const vector<string>& handles, const string& public_key) = 0;
    virtual set<string> links_exist(const vector<string>& handles, const string& public_key) = 0;

    virtual string add_atom(const atoms::Atom* atom,
                            const string& public_key,
                            bool throw_if_exists = false) = 0;
    virtual string add_node(const atoms::Node* node,
                            const string& public_key,
                            bool throw_if_exists = false) = 0;
    virtual string add_link(const atoms::Link* link,
                            const string& public_key,
                            bool throw_if_exists = false) = 0;

    virtual vector<string> add_atoms(const vector<atoms::Atom*>& atoms,
                                     const string& public_key,
                                     bool throw_if_exists = false,
                                     bool is_transactional = false) = 0;
    virtual vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                                     const string& public_key,
                                     bool throw_if_exists = false,
                                     bool is_transactional = false) = 0;
    virtual vector<string> add_links(const vector<atoms::Link*>& links,
                                     const string& public_key,
                                     bool throw_if_exists = false,
                                     bool is_transactional = false) = 0;

    virtual bool delete_atom(const string& handle,
                             const string& public_key,
                             bool delete_link_targets = false) = 0;
    virtual bool delete_node(const string& handle,
                             const string& public_key,
                             bool delete_link_targets = false) = 0;
    virtual bool delete_link(const string& handle,
                             const string& public_key,
                             bool delete_link_targets = false) = 0;

    virtual uint delete_atoms(const vector<string>& handles,
                              const string& public_key,
                              bool delete_link_targets = false) = 0;
    virtual uint delete_nodes(const vector<string>& handles,
                              const string& public_key,
                              bool delete_link_targets = false) = 0;
    virtual uint delete_links(const vector<string>& handles,
                              const string& public_key,
                              bool delete_link_targets = false) = 0;

    virtual void re_index_patterns(const string& public_key, bool flush_patterns = true) = 0;

    virtual size_t node_count(const string& public_key) const = 0;
    virtual size_t link_count(const string& public_key) const = 0;
    virtual size_t atom_count(const string& public_key) const = 0;

    bool empty(const string& public_key) const { return atom_count(public_key) == 0; }
};

}  // namespace atomdb
