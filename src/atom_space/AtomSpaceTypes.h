#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "AtomDBAPITypes.h"
#include "HandleTrie.h"
#include "expression_hasher.h"

#define HANDLE_SIZE 32
#define MINIMUM_TARGETS_SIZE 1

using namespace std;
using namespace commons;
using namespace atomdb::atomdb_api_types;

namespace atomspace {

struct HandleDeleter {
    void operator()(char* ptr) const { free(ptr); }
};

struct TargetHandlesDeleter {
    size_t size;
    bool delete_targets = true;  ///< If true, delete the target handles.

    TargetHandlesDeleter(size_t s, bool delete_targets = true)
        : size(s), delete_targets(delete_targets) {}

    void operator()(char** ptr) const {
        if (ptr) {
            for (size_t i = 0; i < size; ++i) {
                if (ptr[i]) {
                    if (delete_targets) {
                        delete[] ptr[i];
                    }
                }
            }
            delete[] ptr;
        }
    }
};

// -------------------------------------------------------------------------------------------------
class Atom : public HandleTrie::TrieValue {
   public:
    string type;                            ///< The type of the atom.
    CustomAttributesMap custom_attributes;  ///< Custom attributes for the atom.

    /**
     * @brief Construct an Atom.
     * @param type The type of the atom.
     * @throws std::runtime_error if type is empty.
     */
    Atom(const string& type, const CustomAttributesMap& custom_attributes = {})
        : type(type), custom_attributes(custom_attributes) {
        this->validate();
    }

    virtual void validate() const {
        if (this->type.empty()) {
            throw runtime_error("Atom type must not be empty");
        }
    }

    virtual ~Atom() override = default;

    virtual string to_string() const {
        return "Atom(type: '" + this->type +
               "', custom_attributes: " + this->custom_attributes.to_string() + ")";
    }

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
    Node(const string& type, const string& name, const CustomAttributesMap& custom_attributes = {})
        : Atom(type, custom_attributes), name(name) {
        this->validate();
    }

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
        return "Node(type: '" + this->type + "', name: '" + this->name +
               "', custom_attributes: " + this->custom_attributes.to_string() + ")";
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
    Link(const string& type,
         const vector<const Atom*>& targets,
         const CustomAttributesMap& custom_attributes = {})
        : Atom(type, custom_attributes), targets(targets) {
        this->validate();
    }

    /**
     * @brief Construct a Link by moving the targets vector.
     *
     * @param type The type of the link.
     * @param targets The target atoms of the link (rvalue reference, will be moved).
     * @throws std::runtime_error if type is empty or targets.size() < MINIMUM_TARGETS_SIZE.
     */
    Link(const string& type,
         vector<const Atom*>&& targets,
         const CustomAttributesMap& custom_attributes = {})
        : Atom(type, custom_attributes), targets(move(targets)) {
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
        string result = "Link(type: '" + this->type + "', targets: [";
        if (!this->targets.empty()) {
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
        }
        result += "], custom_attributes: " + this->custom_attributes.to_string() + ")";
        return result;
    }

    /**
     * @brief Convert a vector of Atom pointers to an array of handles.
     *
     * For each Atom in the input vector, computes its handle (allocating a new C-string for each).
     *
     * @param targets The vector of Atom pointers.
     * @return A newly allocated array of C-string handles (char**). Caller is responsible for freeing
     *         each handle with free() and the array itself with delete[].
     * @throws std::runtime_error if a target is not a Node or Link.
     */
    static char** targets_to_handles(const vector<const Atom*>& targets) {
        char** handles = new char*[targets.size()];  // Will be freed later by the caller
        size_t i = 0;
        for (const auto& target : targets) {
            if (const Node* node = dynamic_cast<const Node*>(target)) {
                handles[i++] = Node::compute_handle(node->type, node->name);
            } else if (const Link* link = dynamic_cast<const Link*>(target)) {
                handles[i++] = Link::compute_handle(link->type, link->targets);
            } else {
                throw runtime_error("Unsupported target type in Link::targets_to_handles");
            }
        }
        return handles;
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
        unique_ptr<char*[], TargetHandlesDeleter> targets_handles(Link::targets_to_handles(targets),
                                                                  TargetHandlesDeleter(targets.size()));

        return Link::compute_handle(type.c_str(), targets_handles.get(), targets.size());
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
        unique_ptr<char*[], TargetHandlesDeleter> targets_handles(
            new char*[targets.size()], TargetHandlesDeleter(targets.size(), false));
        size_t i = 0;
        for (const auto& target : targets) targets_handles[i++] = (char*) target.c_str();
        return Link::compute_handle(type.c_str(), targets_handles.get(), targets.size());
    }

    /**
     * @brief Compute the handle for a Link given its type hash and target handles.
     *
     * This private static utility computes a handle for a Link using the provided type hash and an array
     * of target handles. Used internally by the public compute_handle methods.
     *
     * @param type The link type.
     * @param targets_handles Array of target atom handles (dynamically allocated, will be freed).
     * @param targets_size Number of target atoms.
     * @return A newly allocated char array containing the handle (caller must free).
     * @throws std::runtime_error if type or targets_handles is null, if type size is too
     *                            small, or if targets_size is less than MINIMUM_TARGETS_SIZE.
     */
    static char* compute_handle(const char* type, char** targets_handles, size_t targets_size) {
        if (type == nullptr || targets_handles == nullptr) {
            throw runtime_error("Type and targets handles must not be null");
        }
        if (strlen(type) == 0) {
            throw runtime_error("Type size is too small");
        }
        if (targets_size < MINIMUM_TARGETS_SIZE) {
            throw runtime_error("Link must have at least " + std::to_string(MINIMUM_TARGETS_SIZE) +
                                " targets");
        }
        unique_ptr<char, HandleDeleter> type_hash(named_type_hash((char*) type));
        return expression_hash(type_hash.get(), targets_handles, targets_size);
    }
};

}  // namespace atomspace
