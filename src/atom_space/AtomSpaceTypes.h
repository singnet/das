#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "HandleTrie.h"
#include "expression_hasher.h"

#define HANDLE_SIZE 32
#define MINIMUM_TARGETS_SIZE 1

using namespace std;
using namespace commons;

namespace atomspace {

// -------------------------------------------------------------------------------------------------
class Atom : public HandleTrie::TrieValue {
   public:
    string type;  ///< The type of the atom.

    /**
     * @brief Construct an Atom.
     * @param type The type of the atom.
     * @throws std::runtime_error if type is empty.
     */
    Atom(const string& type) : type(type) { this->validate(); }

    virtual void validate() const {
        if (this->type.empty()) {
            throw runtime_error("Atom type must not be empty");
        }
    }

    virtual ~Atom() override = default;

    virtual string to_string() const { return "Atom(type: " + this->type + ")"; }

    virtual void merge(HandleTrie::TrieValue* other) override {}
};

// -------------------------------------------------------------------------------------------------
class Node : public Atom {
   public:
    string name;  ///< The name of the node.

    /**
     * @brief Construct a Node.
     * @param type The type of the node.
     * @param name The name of the node.
     * @throws std::runtime_error if type or name is empty.
     */
    Node(const string& type, const string& name) : Atom(type), name(name) { this->validate(); }

    /**
     * @brief Validate the Node object.
     *
     * Checks that the type is valid (by calling Atom::validate()) and that the name is not empty.
     *
     * @throws std::runtime_error if the name is empty.
     */
    virtual void validate() const override {
        Atom::validate();  // Validate type
        if (this->name.empty()) {
            throw runtime_error("Node name must not be empty");
        }
    }

    virtual string to_string() const {
        return "Node(type: " + this->type + ", name: " + this->name + ")";
    }

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
    vector<const Atom*> targets;  ///< The target atoms of the link.

    /**
     * @brief Construct a Link.
     * @param type The type of the link.
     * @param targets The target atoms of the link.
     * @param delete_targets If true, Link will delete its targets on destruction.
     * @throws std::runtime_error if type is empty or targets.size() < MINIMUM_TARGETS_SIZE.
     */
    Link(const string& type, const vector<const Atom*>& targets) : Atom(type), targets(targets) {
        this->validate();
    }

    /**
     * @brief Construct a Link by moving the targets vector.
     *
     * @param type The type of the link.
     * @param targets The target atoms of the link (rvalue reference, will be moved).
     * @throws std::runtime_error if type is empty or targets.size() < MINIMUM_TARGETS_SIZE.
     */
    Link(const string& type, vector<const Atom*>&& targets) : Atom(type), targets(move(targets)) {
        this->validate();
    }

    /**
     * @brief Validate the Link object.
     *
     * Checks that the type is valid (by calling Atom::validate()) and that the number of targets
     * is at least MINIMUM_TARGETS_SIZE.
     *
     * @throws std::runtime_error if the number of targets is less than MINIMUM_TARGETS_SIZE.
     */
    void validate() const {
        Atom::validate();  // Validate type
        if (this->targets.size() < MINIMUM_TARGETS_SIZE) {
            throw runtime_error("Link must have at least " + std::to_string(MINIMUM_TARGETS_SIZE) +
                                " targets");
        }
    }

    /**
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
    static char* compute_handle(const string& type, const vector<const Atom*>& targets) {
        char* link_type_hash = named_type_hash((char*) type.c_str());  // Will be freed later
        char** targets_handles = new char*[targets.size()];            // Will be freed later
        size_t i = 0;
        for (const auto& target : targets) {
            if (const Node* node = dynamic_cast<const Node*>(target)) {
                targets_handles[i++] = Node::compute_handle(node->type, node->name);
            } else if (const Link* link = dynamic_cast<const Link*>(target)) {
                targets_handles[i++] = Link::compute_handle(link->type, link->targets);
            } else {
                throw runtime_error("Unsupported target type in Link::compute_handle");
            }
        }
        auto link = Link::compute_handle(link_type_hash, targets_handles, targets.size());
        free(link_type_hash);      // Clean up the dynamically allocated hash
        delete[] targets_handles;  // Clean up the dynamically allocated array
        return link;
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
        auto link = Link::compute_handle(link_type_hash, targets_handles, targets.size());
        free(link_type_hash);      // Clean up the dynamically allocated hash
        delete[] targets_handles;  // Clean up the dynamically allocated array
        return link;
    }

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
     */
    static char* compute_handle(char* type_hash, char** targets_handles, size_t targets_size) {
        if (type_hash == nullptr || targets_handles == nullptr) {
            throw runtime_error("Type hash and targets handles must not be null");
        }
        if (strlen(type_hash) < HANDLE_SIZE) {
            throw runtime_error("Type hash size is too small");
        }
        if (targets_size < MINIMUM_TARGETS_SIZE) {
            throw runtime_error("Link must have at least " + std::to_string(MINIMUM_TARGETS_SIZE) +
                                " targets");
        }
        return expression_hash(type_hash, targets_handles, targets_size);
    }
};

}  // namespace atomspace
