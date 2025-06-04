#pragma once

#include <memory>

#include "HandleTrie.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBus.h"
#include "expression_hasher.h"

#define HANDLE_SIZE 32
#define IGNORE_ANSWER_COUNT 0
#define MINIMUM_TARGETS_SIZE 1

using namespace std;
using namespace commons;
using namespace service_bus;
using namespace query_engine;

namespace atomspace {

// -------------------------------------------------------------------------------------------------
class Atom : public HandleTrie::TrieValue {
   public:
    virtual ~Atom() override = default;

    virtual string to_string() override { return "Atom"; }

    virtual void merge(HandleTrie::TrieValue* other) override {}
};

// -------------------------------------------------------------------------------------------------
class Node : public Atom {
   public:
    string type;  ///< The type of the node.
    string name;  ///< The name of the node.

    /**
     * @brief Construct a Node.
     * @param type The type of the node.
     * @param name The name of the node.
     * @throws std::runtime_error if type or name is empty.
     */
    Node(const string& type, const string& name) : type(type), name(name) {
        if (this->type.empty() || this->name.empty()) {
            throw runtime_error("Node type and name must not be empty");
        }
    }

    virtual string to_string() override {
        return "Node(type: " + this->type + ", name: " + this->name + ")";
    }

    void merge(HandleTrie::TrieValue* other) override {}

    /**
     * @brief Compute the handle for a Node given its type and name.
     *
     * This static utility computes a handle for a Node using the provided type and name,
     * without requiring an instantiated Node object.
     *
     * @param type The type of the node.
     * @param name The name of the node.
     * @return A newly allocated char array containing the handle (caller must free).
     * @note Caller is responsible for freeing the returned handle.
     */
    static char* compute_handle(const string& type, const string& name) {
        if (type.empty() || name.empty()) {
            throw runtime_error("Node type and name must not be empty");
        }
        return terminal_hash((char*) type.c_str(), (char*) name.c_str());
    }
};

// -------------------------------------------------------------------------------------------------
class Link : public Atom {
   public:
    string type;            ///< The type of the link.
    vector<Atom*> targets;  ///< The target atoms of the link.

    /**
     * @brief Construct a Link.
     * @param type The type of the link.
     * @param targets The target atoms of the link.
     * @param delete_targets If true, Link will delete its targets on destruction.
     * @throws std::runtime_error if type is empty or targets.size() < MINIMUM_TARGETS_SIZE.
     */
    Link(const string& type, const vector<Atom*>& targets) : type(type), targets(targets) {
        if (this->type.empty()) {
            throw runtime_error("Link type must not be empty");
        }
        if (this->targets.size() < MINIMUM_TARGETS_SIZE) {
            throw runtime_error("Link must have at least " + std::to_string(MINIMUM_TARGETS_SIZE) +
                                " targets");
        }
    }

    /**
     * @throws std::runtime_error if a target is not a Node or Link.
     */
    virtual string to_string() override {
        string result = "Link(type: " + this->type + ", targets: [";
        for (const auto& target : this->targets) {
            if (Node* node = dynamic_cast<Node*>(target)) {
                result += node->to_string() + ", ";
            } else if (Link* link = dynamic_cast<Link*>(target)) {
                result += link->to_string() + ", ";
            } else {
                throw runtime_error("Unsupported target type in Link::to_string");
            }
        }
        result.erase(result.size() - 2);  // Remove the last comma and space
        result += "])";
        return result;
    }

    void merge(HandleTrie::TrieValue* other) override {}

    /**
     * @brief Compute the handle for a Link given its type and targets.
     *
     * This static utility computes a handle for a Link using the provided type and target atoms,
     * without requiring an instantiated Link object.
     *
     * @param type The type of the link.
     * @param targets The target atoms of the link.
     * @return A newly allocated char array containing the handle (caller must free).
     * @throws std::runtime_error if a target is not a Node or Link.
     * @note Caller is responsible for freeing the returned handle.
     */
    static char* compute_handle(const string& type, const vector<Atom*>& targets) {
        char* link_type_hash = named_type_hash((char*) type.c_str());  // Will be freed later
        char** targets_handles = new char*[targets.size()];            // Will be freed later
        size_t i = 0;
        for (const auto& target : targets) {
            if (Node* node = dynamic_cast<Node*>(target)) {
                targets_handles[i++] = Node::compute_handle(node->type, node->name);
            } else if (Link* link = dynamic_cast<Link*>(target)) {
                targets_handles[i++] = Link::compute_handle(link->type, link->targets);
            } else {
                throw runtime_error("Unsupported target type in Link::compute_handle");
            }
        }
        return Link::_compute_handle(link_type_hash, targets_handles, targets.size());
    }

    /**
     * @brief Compute the handle for a Link given its type and targets (string handles).
     *
     * This static utility computes a handle for a Link using the provided type and a vector of target
     * handles (as strings), without requiring an instantiated Link object.
     *
     * @param type The type of the link.
     * @param targets The handles of the target atoms as strings.
     * @return A newly allocated char array containing the handle (caller must free).
     * @note Caller is responsible for freeing the returned handle.
     */
    static char* compute_handle(const string& type, const vector<string>& targets) {
        char* link_type_hash = named_type_hash((char*) type.c_str());  // Will be freed later
        char** targets_handles = new char*[targets.size()];            // Will be freed later
        size_t i = 0;
        for (const auto& target : targets) targets_handles[i++] = (char*) target.c_str();
        return Link::_compute_handle(link_type_hash, targets_handles, targets.size());
    }

   private:
    /**
     * @brief Compute the handle for a Link given its type hash and target handles.
     *
     * This private static utility computes a handle for a Link using the provided type hash and an array
     * of target handles. Used internally by the public compute_handle methods.
     *
     * @param type_hash The hash of the link type (dynamically allocated, will be freed).
     * @param targets_handles Array of target atom handles (dynamically allocated, will be freed).
     * @param targets_size Number of target atoms.
     * @return A newly allocated char array containing the handle (caller must free).
     * @throws std::runtime_error if type_hash or targets_handles is null, if type_hash size is too
     * small, or if targets_size is less than MINIMUM_TARGETS_SIZE.
     * @note This function frees type_hash and targets_handles.
     */
    static char* _compute_handle(char* type_hash, char** targets_handles, unsigned int targets_size) {
        if (type_hash == nullptr || targets_handles == nullptr) {
            throw runtime_error("Type hash and targets handles must not be null");
        }
        if (sizeof(type_hash) < HANDLE_SIZE) {
            throw runtime_error("Type hash size is too small");
        }
        if (targets_size < MINIMUM_TARGETS_SIZE) {
            throw runtime_error("Link must have at least " + std::to_string(MINIMUM_TARGETS_SIZE) +
                                " targets");
        }
        char* handle = expression_hash(type_hash, targets_handles, targets_size);
        free(type_hash);           // Clean up the dynamically allocated hash
        delete[] targets_handles;  // Clean up the dynamically allocated array
        return handle;
    }
};

// -------------------------------------------------------------------------------------------------
/**
 * @brief AtomSpace class for managing atoms, nodes, and links.
 *
 * This class provides methods to retrieve, add, and commit atoms, nodes, and links,
 * as well as to perform pattern matching queries via a service bus.
 */
class AtomSpace {
   public:
    /**
     * @brief Scope for atom retrieval and commit operations.
     * LOCAL_ONLY: Only search or commit locally.
     * REMOTE_ONLY: Only search or commit remotely.
     * LOCAL_AND_REMOTE: Try local first, then remote if not found.
     */
    enum Scope { LOCAL_ONLY, REMOTE_ONLY, LOCAL_AND_REMOTE };

    /**
     * @brief Construct an AtomSpace with a given ServiceBus.
     * @param bus Shared pointer to the ServiceBus instance.
     */
    AtomSpace(shared_ptr<ServiceBus> bus);

    /**
     * @brief Retrieve an atom by handle.
     *
     * LOCAL_ONLY and REMOTE_ONLY look for the atom in db or by issuing a service bus command
     * accordingly. For LOCAL_AND_REMOTE, look for the atom in the db. If present, return it.
     * Otherwise, issue a command in the bus.
     *
     * @param handle The atom handle.
     * @param scope Where to search for the atom (default: LOCAL_AND_REMOTE).
     * @return Pointer to the Atom, or nullptr if not found.
     */
    const Atom* get_atom(const char* handle, Scope scope = LOCAL_AND_REMOTE);

    /**
     * @brief Retrieve a node by type and name.
     *
     * LOCAL_ONLY and REMOTE_ONLY look for the node in db or by issuing a service bus command
     * accordingly. For LOCAL_AND_REMOTE, look for the node in the db. If present, return it.
     * Otherwise, issue a command in the bus.
     *
     * @param type Node type.
     * @param name Node name.
     * @param scope Where to search for the node (default: LOCAL_AND_REMOTE).
     * @return Pointer to the Node, or nullptr if not found.
     */
    const Node* get_node(const string& type, const string& name, Scope scope = LOCAL_AND_REMOTE);

    /**
     * @brief Retrieve a link by type and targets.
     *
     * LOCAL_ONLY and REMOTE_ONLY look for the link in db or by issuing a service bus command
     * accordingly. For LOCAL_AND_REMOTE, look for the link in the db. If present, return it.
     * Otherwise, issue a command in the bus.
     *
     * @param type Link type.
     * @param targets List of target atoms.
     * @param scope Where to search for the link (default: LOCAL_AND_REMOTE).
     * @return Pointer to the Link, or nullptr if not found.
     */
    const Link* get_link(const string& type,
                         const vector<Atom*>& targets,
                         Scope scope = LOCAL_AND_REMOTE);

    /**
     * @brief Issue a pattern matching query via the service bus.
     *
     * @param query The query pattern.
     * @param answers_count Maximum number of answers to return (0 = all).
     * @param context Optional context string.
     * @param unique_assignment Whether to enforce unique variable assignments.
     * @param update_attention_broker Whether to update the attention broker.
     * @param count_only If true, only count the matches.
     * @return Shared pointer to the PatternMatchingQueryProxy.
     */
    shared_ptr<PatternMatchingQueryProxy> pattern_matching_query(
        const vector<string>& query,
        unsigned int answers_count = IGNORE_ANSWER_COUNT,
        const string& context = "",
        bool unique_assignment = true,
        bool update_attention_broker = false,
        bool count_only = false);

    /**
     * @brief Count the number of pattern matches via the service bus.
     *
     * @param query The query pattern.
     * @param context Optional context string.
     * @param unique_assignment Whether to enforce unique variable assignments.
     * @param update_attention_broker Whether to update the attention broker.
     * @return Number of matches.
     */
    unsigned int pattern_matching_count(const vector<string>& query,
                                        const string& context = "",
                                        bool unique_assignment = true,
                                        bool update_attention_broker = false);

    /**
     * @brief Fetch Atoms of a pattern matching results from the remote database and store them
     * locally.
     *
     * @param query The query pattern.
     * @param answers_count Maximum number of answers to fetch (0 = all).
     */
    void pattern_matching_fetch(const vector<string>& query,
                                unsigned int answers_count = IGNORE_ANSWER_COUNT);

    /**
     * @brief Add a node to the AtomSpace.
     *
     * Atoms are instantiated inside add_node(). Atom handle is computed (and NOT stored in the Atom
     * object). Atom pointer is stored directly in the HandleTrie using the computed handle as key.
     *
     * @param type Node type.
     * @param name Node name.
     * @return The handle of the new node (caller is responsible for freeing).
     */
    char* add_node(const string& type, const string& name);

    /**
     * @brief Add a link to the AtomSpace.
     *
     * Atoms are instantiated inside add_link(). Atom handle is computed (and NOT stored in the Atom
     * object). Atom pointer is stored directly in the HandleTrie using the computed handle as key.
     *
     * @param type Link type.
     * @param targets List of target atoms.
     * @return The handle of the new link (caller is responsible for freeing).
     */
    char* add_link(const string& type, const vector<Atom*>& targets);

    /**
     * @brief Commit changes to the AtomSpace.
     *
     * If LOCAL scope is selected (LOCAL_ONLY or LOCAL_AND_REMOTE), commit ALL atoms in local db by
     * sending them to the server (by issuing a command in the service bus). Atoms should be kept in
     * the local db. The command should wait this sync ends before going to the next step. Once local
     * changes are committed, issue a DB COMMIT command in the service bus.
     *
     * @param scope Where to commit changes (default: LOCAL_AND_REMOTE).
     */
    void commit_changes(Scope scope = LOCAL_AND_REMOTE);

   private:
    shared_ptr<ServiceBus> bus;
    unique_ptr<HandleTrie> handle_trie;
};

}  // namespace atomspace
