#pragma once

#include <memory>

#include "HandleTrie.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBus.h"
#include "expression_hasher.h"

#define HANDLE_SIZE 32
#define IGNORE_ANSWER_COUNT 0
#define MINIMUM_TARGETS_SIZE 2

using namespace std;
using namespace commons;
using namespace service_bus;
using namespace query_engine;

namespace atomspace {

class Atom : public HandleTrie::TrieValue {
   public:
    /**
     * @brief Virtual destructor for Atom.
     */
    virtual ~Atom() override = default;

    /**
     * @brief Get a string representation of the Atom.
     * @return A string describing the Atom.
     */
    virtual string to_string() override { return "Atom"; }

    /**
     * @brief Merge another TrieValue into this Atom.
     * @param other The other TrieValue.
     * @throws std::runtime_error Always, as merge is not implemented for Atom.
     */
    virtual void merge(HandleTrie::TrieValue* other) override {
        throw runtime_error("Merge not implemented for Atom");
    }

    /**
     * @brief Compute the handle for this Atom.
     *
     * The handle is not stored in the Atom object and must be implemented by derived classes.
     *
     * @return A newly allocated char array containing the handle (caller must free).
     */
    virtual char* compute_handle() const = 0;
};

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

    /**
     * @brief Get a string representation of the Node.
     * @return A string describing the Node.
     */
    virtual string to_string() const {
        return "Node(type: " + this->type + ", name: " + this->name + ")";
    }

    /**
     * @brief Merge another TrieValue into this Node.
     * @param other The other TrieValue.
     * @throws std::runtime_error Always, as merge is not implemented for Node.
     */
    void merge(HandleTrie::TrieValue* other) override {
        throw runtime_error("Merge not implemented for Node");
    }

    /**
     * @brief Compute the handle for this Node.
     *
     * The handle is computed from the node type and name.
     * The handle is not stored in the Node object.
     *
     * @return A newly allocated char array containing the handle (caller must free).
     *
     * @note Caller is responsible for freeing the returned handle.
     */
    virtual char* compute_handle() const override {
        return terminal_hash((char*) this->type.c_str(), (char*) this->name.c_str());
    }
};

class Link : public Atom {
   public:
    string type;            ///< The type of the link.
    vector<Atom*> targets;  ///< The target atoms of the link.
    bool delete_targets;    ///< Whether to delete target atoms on destruction.

    /**
     * @brief Construct a Link.
     * @param type The type of the link.
     * @param targets The target atoms of the link.
     * @param delete_targets If true, Link will delete its targets on destruction.
     * @throws std::runtime_error if type is empty or targets.size() < MINIMUM_TARGETS_SIZE.
     */
    Link(const string& type, const vector<Atom*>& targets, bool delete_targets = false)
        : type(type), targets(targets), delete_targets(delete_targets) {
        if (this->type.empty()) {
            throw runtime_error("Link type must not be empty");
        }
        if (this->targets.size() < MINIMUM_TARGETS_SIZE) {
            throw runtime_error("Link must have at least " + std::to_string(MINIMUM_TARGETS_SIZE) +
                                " targets");
        }
    }

    /**
     * @brief Destructor for Link.
     *
     * Deletes target atoms if delete_targets is true.
     */
    virtual ~Link() {
        if (delete_targets)
            for (auto& target : targets) delete target;
    }

    /**
     * @brief Get a string representation of the Link.
     * @return A string describing the Link and its targets.
     * @throws std::runtime_error if a target is not a Node or Link.
     */
    virtual string to_string() const {
        string result = "Link(type: " + this->type + ", targets: [";
        for (const auto& target : this->targets) {
            if (const Node* node = dynamic_cast<const Node*>(target)) {
                result += node->to_string() + ", ";
            } else if (const Link* link = dynamic_cast<const Link*>(target)) {
                result += link->to_string() + ", ";
            } else {
                throw runtime_error("Unsupported target type in Link::to_string");
            }
        }
        result.erase(result.size() - 2);  // Remove the last comma and space
        result += "])";
        return result;
    }

    /**
     * @brief Merge another TrieValue into this Link.
     * @param other The other TrieValue.
     * @throws std::runtime_error Always, as merge is not implemented for Link.
     */
    void merge(HandleTrie::TrieValue* other) override {
        throw runtime_error("Merge not implemented for Link");
    }

    /**
     * @brief Compute the handle for this Link.
     *
     * The handle is computed from the link type and the handles of its targets.
     * The handle is not stored in the Link object.
     *
     * @return A newly allocated char array containing the handle (caller must free).
     * @throws std::runtime_error if a target is not a Node or Link.
     *
     * @note Caller is responsible for freeing the returned handle.
     */
    virtual char* compute_handle() const override {
        char* link_type_hash = named_type_hash((char*) this->type.c_str());
        char** targets_handles = new char*[this->targets.size()];
        for (size_t i = 0; i < this->targets.size(); ++i) {
            if (const Node* node = dynamic_cast<const Node*>(this->targets[i])) {
                targets_handles[i] = node->compute_handle();
            } else if (const Link* link = dynamic_cast<const Link*>(this->targets[i])) {
                targets_handles[i] = link->compute_handle();
            } else {
                throw runtime_error("Unsupported target type in Link::compute_handle");
            }
        }
        char* handle = expression_hash(link_type_hash, targets_handles, this->targets.size());
        delete link_type_hash;            // Clean up the dynamically allocated hash
        for (size_t i = 0; i < this->targets.size(); ++i) {
            delete[] targets_handles[i];  // Clean up each target handle
        }
        delete[] targets_handles;         // Clean up the dynamically allocated array
        return handle;
    }
};

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
    AtomSpace(shared_ptr<ServiceBus> bus) : bus(bus) {
        this->handle_trie = make_unique<HandleTrie>(HANDLE_SIZE);
    }

    /**
     * @brief Retrieve an atom by handle.
     *
     * LOCAL_ONLY and REMOTE_ONLY look for the atom in db or by issuing a service bus command
     * accordingly. For LOCAL_AND_REMOTE, look for the atom in the db. If present, return it. Otherwise,
     * issue a command in the bus.
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
     * accordingly. For LOCAL_AND_REMOTE, look for the node in the db. If present, return it. Otherwise,
     * issue a command in the bus.
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
     * accordingly. For LOCAL_AND_REMOTE, look for the link in the db. If present, return it. Otherwise,
     * issue a command in the bus.
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
     * @brief Fetch Atoms of a pattern matching results from the remote database and store them locally.
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
     * sending them to the server (by issuing a command in the service bus). Atoms should be kept in the
     * local db. The command should wait this sync ends before going to the next step. Once local changes
     * are committed, issue a DB COMMIT command in the service bus.
     *
     * @param scope Where to commit changes (default: LOCAL_AND_REMOTE).
     */
    void commit_changes(Scope scope = LOCAL_AND_REMOTE);

   private:
    shared_ptr<ServiceBus> bus;
    unique_ptr<HandleTrie> handle_trie;
};

}  // namespace atomspace