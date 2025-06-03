#include "AtomSpace.h"

using namespace std;
using namespace query_engine;

namespace atomspace {

// -------------------------------------------------------------------------------------------------
const Atom* AtomSpace::get_atom(const char* handle, Scope scope) {
    if (scope == LOCAL_ONLY || scope == LOCAL_AND_REMOTE) {
        auto atom = this->handle_trie->lookup(handle);
        if (atom) return dynamic_cast<const Atom*>(atom);
    }
    // TODO: query the remote database.
    return NULL;
}

// -------------------------------------------------------------------------------------------------
const Node* AtomSpace::get_node(const string& type, const string& name, Scope scope) {
    auto node = this->get_atom(Node(type, name).compute_handle());
    if (node) return dynamic_cast<const Node*>(node);
    return NULL;
}

// -------------------------------------------------------------------------------------------------
const Link* AtomSpace::get_link(const string& type, const vector<Atom*>& targets, Scope scope) {
    auto link = this->get_atom(Link(type, targets).compute_handle());
    if (link) return dynamic_cast<const Link*>(link);
    return NULL;
}

// -------------------------------------------------------------------------------------------------
shared_ptr<PatternMatchingQueryProxy> AtomSpace::pattern_matching_query(const vector<string>& query,
                                                                        unsigned int answers_count,
                                                                        const string& context,
                                                                        bool unique_assignment,
                                                                        bool update_attention_broker,
                                                                        bool count_only) {
    // TODO: Use `answers_count` parameter to limit the number of answers once the functionality is
    // implemented on the server side.
    shared_ptr<PatternMatchingQueryProxy> proxy = make_shared<PatternMatchingQueryProxy>(
        query, context, unique_assignment, update_attention_broker, count_only);
    this->bus->issue_bus_command(proxy);
    return proxy;
}

// -------------------------------------------------------------------------------------------------
unsigned int AtomSpace::pattern_matching_count(const vector<string>& query,
                                               const string& context,
                                               bool unique_assignment,
                                               bool update_attention_broker) {
    auto proxy = this->pattern_matching_query(query,
                                              /*answers_count=*/IGNORE_ANSWER_COUNT,
                                              context,
                                              unique_assignment,
                                              update_attention_broker,
                                              /*count_only=*/true);
    return proxy->get_count();
}

// -------------------------------------------------------------------------------------------------
void AtomSpace::pattern_matching_fetch(const vector<string>& query, unsigned int answers_count) {
    // TODO: Implement the fetch logic once the server-side supports it.
}

// -------------------------------------------------------------------------------------------------
char* AtomSpace::add_node(const string& type, const string& name) {
    Node* node = new Node(type, name);
    char* handle = node->compute_handle();
    this->handle_trie->insert(handle, node);
    return handle;
}

// -------------------------------------------------------------------------------------------------
char* AtomSpace::add_link(const string& type, const vector<Atom*>& targets) {
    Link* link = new Link(type, targets);
    char* handle = link->compute_handle();
    this->handle_trie->insert(handle, link);
    return handle;
}

// -------------------------------------------------------------------------------------------------
void AtomSpace::commit_changes(Scope scope) {
    // TODO: Implement commit logic once the server-side supports it.
}

}  // namespace atomspace