#pragma once

#include <set>
#include <string>
#include <vector>

#include "AtomDBAPITypes.h"
#include "HandleDecoder.h"
#include "Properties.h"

using namespace std;
using namespace commons;

namespace atomdb {

class AtomDB : public HandleDecoder {
   public:
    AtomDB() = default;
    virtual ~AtomDB() = default;

    static inline string WILDCARD = "*";

    virtual shared_ptr<Atom> get_atom(const string& handle) = 0;  // HandleDecoder interface

    virtual shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(
        const LinkTemplateInterface& link_template) = 0;
    virtual shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle) = 0;
    virtual shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const string& handle) = 0;
    virtual bool link_exists(const string& link_handle) = 0;
    virtual set<string> links_exist(const vector<string>& link_handles) = 0;
    virtual string add_node(const atomspace::Node* node) = 0;
    virtual string add_link(const atomspace::Link* link) = 0;
    virtual vector<shared_ptr<atomdb_api_types::AtomDocument>> get_atom_documents(
        const vector<string>& handles, const vector<string>& fields) = 0;

   private:
    virtual void attention_broker_setup() = 0;
};

}  // namespace atomdb
