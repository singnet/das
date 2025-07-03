#pragma once

#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include "AtomDBAPITypes.h"
#include "HandleDecoder.h"
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

    virtual shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(
        const LinkTemplateInterface& link_template) = 0;
    virtual shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle) = 0;
    virtual shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const string& handle) = 0;
    virtual bool link_exists(const string& link_handle) = 0;
    virtual set<string> links_exist(const vector<string>& link_handles) = 0;
    virtual vector<shared_ptr<atomdb_api_types::AtomDocument>> get_atom_documents(
        const vector<string>& handles, const vector<string>& fields) = 0;

    template <typename T>
    string add_atom(const T* atom) {
        if constexpr (is_same_v<T, atoms::Node>) {
            return add_node(atom);
        } else if constexpr (is_same_v<T, atoms::Link>) {
            return add_link(atom);
        } else {
            Utils::error("Unsupported atom type.");
        }
    }

    template <typename T>
    vector<string> add_atoms(const vector<T*>& atoms) {
        if constexpr (is_same_v<T, atoms::Node>) {
            return add_nodes(atoms);
        } else if constexpr (is_same_v<T, atoms::Link>) {
            return add_links(atoms);
        } else {
            Utils::error("Unsupported atom type.");
        }
    }

    template <typename T>
    bool delete_atom(const string& handle) {
        if constexpr (is_same_v<T, atoms::Node>) {
            return delete_node(handle);
        } else if constexpr (is_same_v<T, atoms::Link>) {
            return delete_link(handle);
        } else {
            Utils::error("Unsupported atom type.");
        }
    }

    template <typename T>
    uint delete_atoms(const vector<string>& handles) {
        if constexpr (is_same_v<T, atoms::Node>) {
            return delete_nodes(handles);
        } else if constexpr (is_same_v<T, atoms::Link>) {
            return delete_links(handles);
        } else {
            Utils::error("Unsupported atom type.");
        }
    }

   protected:
    virtual string add_node(const atoms::Node* node) = 0;
    virtual vector<string> add_nodes(const vector<atoms::Node*>& nodes) = 0;
    virtual bool delete_node(const string& handle) = 0;
    virtual uint delete_nodes(const vector<string>& handles) = 0;

    virtual string add_link(const atoms::Link* link) = 0;
    virtual vector<string> add_links(const vector<atoms::Link*>& links) = 0;
    virtual bool delete_link(const string& handle) = 0;
    virtual uint delete_links(const vector<string>& handles) = 0;

   private:
    virtual void attention_broker_setup() = 0;
};

}  // namespace atomdb
