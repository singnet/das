#pragma once

#include <string>
#include <vector>

#include "Atom.h"
#include "QueryElement.h"

using namespace std;
using namespace query_engine;
using namespace atoms;

namespace query_element {

/**
 * A QueryElement which represents terminals (i.e. Nodes, Links and Variables) in the query tree.
 */
class Terminal : public QueryElement {
   public:
    bool is_variable;
    bool is_link;
    bool is_node;
    bool is_atom;
    string type;
    string name;
    string handle;
    vector<shared_ptr<QueryElement>> targets;

    ~Terminal(){};
    Terminal();                                                                     // Atom
    Terminal(const string& type, const string& name);                               // Node
    Terminal(const string& type, const vector<shared_ptr<QueryElement>>& targets);  // Link
    Terminal(const string& name);                                                   // Variable
    string to_string();

    // QueryElement virtual API

    /**
     * Empty implementation. There are no QueryNode element to setup.
     */
    void setup_buffers() {}

    /**
     * Empty implementation. There are no QueryNode element or local thread to shut down.
     */
    void graceful_shutdown() {}

   private:
    void init();
};
}  // namespace query_element
