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
const Link* AtomSpace::get_link(const string& type, const vector<const Atom*>& targets, Scope scope) {
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
                                                                        size_t answers_count,
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
size_t AtomSpace::pattern_matching_count(const vector<string>& query,
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
void AtomSpace::pattern_matching_fetch(const vector<string>& query, size_t answers_count) {
    auto proxy = this->pattern_matching_query(query, answers_count);
    this->bus->issue_bus_command(proxy);
    shared_ptr<QueryAnswer> query_answer;
    const char* handle;
    while (!proxy->finished()) {
        if ((query_answer = proxy->pop()) == NULL) {
            Utils::sleep();
        } else {
            for (size_t i = 0; i < query_answer->handles_size; i++) {
                handle = query_answer->handles[i];
                this->get_atom(handle, LOCAL_AND_REMOTE);  // Ensure the atom is loaded into the trie
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
char* AtomSpace::add_link(const string& type, const vector<const Atom*>& targets) {
    char* handle = Link::compute_handle(type, targets);
    this->handle_trie->insert(handle, new Link(type, targets));
    return handle;
}

// -------------------------------------------------------------------------------------------------
void AtomSpace::commit_changes(Scope scope) {
    /*
    If LOCAL scope is selected (LOCAL_ONLY or LOCAL_AND_REMOTE), commit ALL atoms in the handle trie
    by sending them to the remote DB. Atoms should be kept in the handle trie.
    The command should wait this sync ends before going to the next step.
    */
    if (scope == LOCAL_ONLY) {
        // Commit only locally, no remote action needed.
        return;
    }
    auto commit_changes_visit_lambda = [](HandleTrie::TrieNode* trie_node, void* user_data) -> bool {
        auto db = static_cast<AtomDB*>(user_data);
        if (const auto node = dynamic_cast<const Node*>(trie_node->value)) {
            db->add_node(node->type.c_str(), node->name.c_str());
        } else if (const auto link = dynamic_cast<const Link*>(trie_node->value)) {
            auto targets_handles = Link::targets_to_handles(link->targets);  // Will be freed later
            db->add_link(link->type.c_str(), targets_handles, link->targets.size());
            for (size_t i = 0; i < link->targets.size(); i++)
                free(targets_handles[i]);  // Clean up the dynamically allocated handles
            delete[] targets_handles;      // Clean up the dynamically allocated array
        } else {
            Utils::error("Unsupported Atom type for commit: " + trie_node->to_string());
            return false;  // Unsupported type, cannot commit
        }
        return true;
    };
    this->handle_trie->traverse(true, commit_changes_visit_lambda, this->db.get());
}

// PRIVATE METHODS /////////////////////////////////////////////////////////////////////////////////

Atom* AtomSpace::atom_from_document(const shared_ptr<AtomDocument>& document) {
    if (document->contains("targets")) {
        // If the document contains targets, it is a Link.
        vector<const Atom*> targets;
        const char* target_handle;
        size_t size = document->get_size("targets");
        for (size_t i = 0; i < size; i++) {
            target_handle = document->get("targets", i);
            auto target_atom = this->get_atom(target_handle, LOCAL_AND_REMOTE);
            if (target_atom) {
                targets.push_back(target_atom);
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
