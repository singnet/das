#include "AtomSpace.h"

#include "QueryAnswer.h"
#include "Utils.h"

using namespace std;
using namespace query_engine;

namespace atomspace {

// -------------------------------------------------------------------------------------------------
AtomSpace::AtomSpace()
    : bus(ServiceBusSingleton::get_instance()),
      handle_trie(make_unique<HandleTrie>(HANDLE_SIZE)),
      db(AtomDBSingleton::get_instance()) {}

// -------------------------------------------------------------------------------------------------
const Atom* AtomSpace::get_atom(const char* handle, Scope scope) {
    if (scope == LOCAL_ONLY || scope == LOCAL_AND_REMOTE) {
        auto atom = this->handle_trie->lookup(handle);
        if (atom) {
            return dynamic_cast<const Atom*>(atom);
        } else if (scope == LOCAL_ONLY) {
            return NULL;  // If LOCAL_ONLY, return NULL if not found locally.
        }
    }
    auto atom_document = this->db->get_atom_document(handle);
    if (atom_document) {
        auto atom = this->atom_from_document(atom_document);
        if (atom) {
            this->handle_trie->insert(handle, atom);
            return atom;
        }
    }
    return NULL;
}

// -------------------------------------------------------------------------------------------------
const Node* AtomSpace::get_node(const string& type, const string& name, Scope scope) {
    auto handle = Node::compute_handle(type, name);
    auto node = this->get_atom(handle, scope);
    if (node) {
        free(handle);
        return dynamic_cast<const Node*>(node);
    }
    free(handle);
    return NULL;
}

// -------------------------------------------------------------------------------------------------
const Link* AtomSpace::get_link(const string& type, const vector<Atom*>& targets, Scope scope) {
    auto handle = Link::compute_handle(type, targets);
    auto link = this->get_atom(handle, scope);
    if (link) {
        free(handle);
        return dynamic_cast<const Link*>(link);
    }
    free(handle);
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
    auto proxy = make_shared<PatternMatchingQueryProxy>(
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
    auto proxy = this->pattern_matching_query(query, answers_count);
    this->bus->issue_bus_command(proxy);
    shared_ptr<QueryAnswer> query_answer;
    while (!proxy->finished()) {
        if ((query_answer = proxy->pop()) == NULL) {
            Utils::sleep();
        } else {
            for (size_t i = 0; i < query_answer->handles_size; i++) {
                const char* handle = query_answer->handles[i];
                auto atom_document = this->db->get_atom_document(handle);
                this->handle_trie->insert(handle, this->atom_from_document(atom_document));
            }
        }
    }
}

// -------------------------------------------------------------------------------------------------
char* AtomSpace::add_node(const string& type, const string& name) {
    char* handle = Node::compute_handle(type, name);
    this->handle_trie->insert(handle, new Node(type, name));
    return handle;
}

// -------------------------------------------------------------------------------------------------
char* AtomSpace::add_link(const string& type, const vector<Atom*>& targets) {
    char* handle = Link::compute_handle(type, targets);
    this->handle_trie->insert(handle, new Link(type, targets));
    return handle;
}

// -------------------------------------------------------------------------------------------------
void AtomSpace::commit_changes(Scope scope) {
    // TODO: Implement commit logic once the server-side supports it.
}

// -------------------------------------------------------------------------------------------------
Atom* AtomSpace::atom_from_document(const shared_ptr<AtomDocument>& document) {
    if (document->contains("targets")) {
        // If the document contains targets, it is a Link.
        vector<Atom*> targets;
        size_t size = document->get_size("targets");
        for (size_t i = 0; i < size; ++i) {
            const char* target_handle = document->get("targets", i);
            auto target_document = this->db->get_atom_document(target_handle);
            if (target_document) {
                targets.push_back(this->atom_from_document(target_document));
            } else {
                throw runtime_error("Target document not found for handle: " + string(target_handle));
            }
        }
        return new Link(document->get("type"), move(targets));
    } else if (document->contains("name")) {
        // If the document contains a name, it is a Node.
        return new Node(document->get("type"), document->get("name"));
    } else {
        // Something unexpected
        throw runtime_error("Invalid AtomDocument: must contain either 'targets' or 'name'");
    }
    return NULL;  // Should never reach here
}

}  // namespace atomspace
