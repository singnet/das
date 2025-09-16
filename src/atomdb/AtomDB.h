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

    virtual shared_ptr<Atom> get_atom(const string& handle) = 0;  // HandleDecoder interface

    virtual shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema) = 0;
    virtual shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle) = 0;
    virtual shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle) = 0;

    virtual shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const string& handle) = 0;
    virtual shared_ptr<atomdb_api_types::AtomDocument> get_node_document(const string& handle) = 0;
    virtual shared_ptr<atomdb_api_types::AtomDocument> get_link_document(const string& handle) = 0;

    virtual vector<shared_ptr<atomdb_api_types::AtomDocument>> get_atom_documents(
        const vector<string>& handles, const vector<string>& fields) = 0;
    virtual vector<shared_ptr<atomdb_api_types::AtomDocument>> get_node_documents(
        const vector<string>& handles, const vector<string>& fields) = 0;
    virtual vector<shared_ptr<atomdb_api_types::AtomDocument>> get_link_documents(
        const vector<string>& handles, const vector<string>& fields) = 0;

    virtual vector<shared_ptr<atomdb_api_types::AtomDocument>> get_matching_atoms(bool is_toplevel,
                                                                                  Atom& key) = 0;

    virtual bool atom_exists(const string& handle) = 0;
    virtual bool node_exists(const string& handle) = 0;
    virtual bool link_exists(const string& handle) = 0;

    virtual set<string> atoms_exist(const vector<string>& handles) = 0;
    virtual set<string> nodes_exist(const vector<string>& handles) = 0;
    virtual set<string> links_exist(const vector<string>& handles) = 0;

    virtual string add_atom(const atoms::Atom* atom) = 0;
    virtual string add_node(const atoms::Node* node) = 0;
    virtual string add_link(const atoms::Link* link) = 0;

    virtual vector<string> add_atoms(const vector<atoms::Atom*>& atoms) = 0;
    virtual vector<string> add_nodes(const vector<atoms::Node*>& nodes) = 0;
    virtual vector<string> add_links(const vector<atoms::Link*>& links) = 0;

    virtual bool delete_atom(const string& handle, bool delete_link_targets = false) = 0;
    virtual bool delete_node(const string& handle, bool delete_link_targets = false) = 0;
    virtual bool delete_link(const string& handle, bool delete_link_targets = false) = 0;

    virtual uint delete_atoms(const vector<string>& handles, bool delete_link_targets = false) = 0;
    virtual uint delete_nodes(const vector<string>& handles, bool delete_link_targets = false) = 0;
    virtual uint delete_links(const vector<string>& handles, bool delete_link_targets = false) = 0;
};

}  // namespace atomdb
