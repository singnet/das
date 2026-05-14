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

    virtual bool allow_nested_indexing() = 0;

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

    virtual string add_atom(const atoms::Atom* atom, bool throw_if_exists = false) = 0;
    virtual string add_node(const atoms::Node* node, bool throw_if_exists = false) = 0;
    virtual string add_link(const atoms::Link* link, bool throw_if_exists = false) = 0;

    virtual vector<string> add_atoms(const vector<atoms::Atom*>& atoms,
                                     bool throw_if_exists = false,
                                     bool is_transactional = false) = 0;
    virtual vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                                     bool throw_if_exists = false,
                                     bool is_transactional = false) = 0;
    virtual vector<string> add_links(const vector<atoms::Link*>& links,
                                     bool throw_if_exists = false,
                                     bool is_transactional = false) = 0;

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

   private:
    void reachable_terminal_set_recursive(set<string>& output,
                                          shared_ptr<Link> link,
                                          bool metta_mapping) {
        bool first_target = true;
        for (string& target_handle : link->targets) {
            auto atom = this->get_atom(target_handle);
            if (Atom::is_node(atom)) {
                if (!(metta_mapping && first_target)) {
                    output.insert(atom->handle());
                }
            } else {
                reachable_terminal_set_recursive(
                    output, dynamic_pointer_cast<Link>(atom), metta_mapping);
            }
            first_target = false;
        }
    }

   public:
    /**
     * The reachable set of a given Link contains any Node in its target list plus any Node
     * reachable through the recursive application of this method to the Links in its
     * targets list. The reachable set of a Node is the Node itself.
     *
     * @param output A std::set where output is supposed to be placed.
     * @param handle The handle of the starting link. If a Node handle is passed
     * instead, the output will be this single handle.
     * @param metta_mapping Optional flag to indicate the use of MeTTa mapping. When MeTTa mapping
     * is being used, the first element in a expression is not considered to be put in the output.
     */
    void reachable_terminal_set(set<string>& output, const string& handle, bool metta_mapping = false) {
        auto atom = this->get_atom(handle);
        if (atom != nullptr) {
            if (Atom::is_node(atom)) {
                output.insert(handle);
            } else {
                reachable_terminal_set_recursive(
                    output, dynamic_pointer_cast<Link>(atom), metta_mapping);
            }
        }
    }
};

}  // namespace atomdb
