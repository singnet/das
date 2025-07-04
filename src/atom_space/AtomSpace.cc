#include "AtomSpace.h"

#include "Hasher.h"
#include "QueryAnswer.h"
#include "Utils.h"

using namespace std;
using namespace query_engine;

namespace atomspace {

// -------------------------------------------------------------------------------------------------
AtomSpace::AtomSpace()
    : db(AtomDBSingleton::get_instance()),
      bus(ServiceBusSingleton::get_instance()),
      handle_trie(make_unique<HandleTrie>(HANDLE_HASH_SIZE - 1)) {}

// -------------------------------------------------------------------------------------------------
const Atom* AtomSpace::get_atom(const char* handle, Scope scope) {
    if (scope == LOCAL_ONLY || scope == LOCAL_AND_REMOTE) {
        auto atom = this->handle_trie->lookup(handle);
        if (atom) {
            return dynamic_cast<const Atom*>(atom);
        } else if (scope == LOCAL_ONLY) {
            return nullptr;  // If LOCAL_ONLY, return nullptr if not found locally.
        }
    }
    // TODO(arturgontijo): Must cover Link case too.
    auto atom_document = this->db->get_atom_document<Node>(handle);
    if (atom_document) {
        auto atom = this->atom_from_document(atom_document);
        if (atom) {
            this->handle_trie->insert(handle, atom);
            return atom;
        }
    }
    return nullptr;
}

// -------------------------------------------------------------------------------------------------
const Node* AtomSpace::get_node(const string& type, const string& name, Scope scope) {
    string handle = Hasher::node_handle(type, name).c_str();
    auto node = this->get_atom(handle.c_str());
    if (node) {
        return dynamic_cast<const Node*>(node);
    } else {
        return nullptr;
    }
}

// -------------------------------------------------------------------------------------------------
const Link* AtomSpace::get_link(const string& type, const vector<const Atom*>& targets, Scope scope) {
    // TODO
    // This API needs to be rewritten
    return nullptr;
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
    auto proxy = make_shared<PatternMatchingQueryProxy>(query, context);
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = unique_assignment;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = update_attention_broker;
    proxy->parameters[PatternMatchingQueryProxy::COUNT_FLAG] = count_only;

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
        if ((query_answer = proxy->pop()) == nullptr) {
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
char* AtomSpace::add_node(const string& type, const string& name, const Properties& custom_attributes) {
    char* handle = ::terminal_hash((char*) type.c_str(), (char*) name.c_str());
    this->handle_trie->insert(handle, new Node(type, name, custom_attributes));
    return handle;
}

// -------------------------------------------------------------------------------------------------
char* AtomSpace::add_link(const string& type,
                          const vector<string>& targets,
                          const Properties& custom_attributes) {
    char* handle = strdup(Hasher::link_handle(type, targets).c_str());
    this->handle_trie->insert(handle, new Link(type, targets, custom_attributes));
    return handle;
}

// -------------------------------------------------------------------------------------------------
void AtomSpace::commit_changes(Scope scope) {
    if (scope == LOCAL_ONLY) {
        // Commit only locally, no remote action needed.
        return;
    }
    auto commit_changes_visit_lambda = [](HandleTrie::TrieNode* trie_node, void* user_data) -> bool {
        auto db = static_cast<AtomDB*>(user_data);
        if (const auto node = dynamic_cast<const Node*>(trie_node->value)) {
            db->add_atom<Node>(node);
        } else if (const auto link = dynamic_cast<const Link*>(trie_node->value)) {
            db->add_atom<Link>(link);
        } else {
            Utils::error("Unsupported Atom type for commit: " + trie_node->to_string());
            return true;  // Unsupported type, cannot commit
        }
        return false;
    };
    this->handle_trie->traverse(true, commit_changes_visit_lambda, this->db.get());
}

// PRIVATE METHODS /////////////////////////////////////////////////////////////////////////////////

Atom* AtomSpace::atom_from_document(const shared_ptr<AtomDocument>& document) {
    if (document->contains("targets")) {
        // If the document contains targets, it is a Link.
        vector<string> targets;
        const char* target_handle;
        size_t size = document->get_size("targets");
        for (size_t i = 0; i < size; i++) {
            target_handle = document->get("targets", i);
            targets.push_back(target_handle);
        }
        return new Link(document->get("type"), targets);
    } else if (document->contains("name")) {
        // If the document contains a name, it is a Node.
        return new Node(document->get("type"), document->get("name"));
    } else {
        // Something unexpected
        throw runtime_error("Invalid AtomDocument: must contain either 'targets' or 'name'");
    }
    return nullptr;  // Should never reach here
}

}  // namespace atomspace
