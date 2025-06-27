#pragma once

#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Atom.h"
#include "HandleDecoder.h"
#include "MettaMapping.h"
#include "Properties.h"
#include "expression_hasher.h"

#define HANDLE_SIZE ((unsigned int) 32)
#define MINIMUM_TARGETS_SIZE ((unsigned int) 1)

using namespace std;
using namespace commons;

namespace atomspace {

// -------------------------------------------------------------------------------------------------
/**
 * @brief Custom deleter for single C-string handles.
 *
 * This struct is intended for use with smart pointers managing single C-string handles
 * (char*), such as those returned by Node::compute_handle or Link::compute_handle.
 * It will free the memory using free().
 *
 * Example usage:
 * @code
 *   std::unique_ptr<char, HandleDeleter> handle(Node::compute_handle(type, name));
 * @endcode
 */
struct HandleDeleter {
    void operator()(char* ptr) const { free(ptr); }
};

// -------------------------------------------------------------------------------------------------
/**
 * @brief Custom deleter for arrays of C-string handles.
 *
 * This struct is intended for use with smart pointers managing arrays of C-string handles
 * (char**), such as those returned by Link::targets_to_handles. It will delete the array itself,
 * and optionally delete each individual C-string handle in the array.
 *
 * @note If `delete_targets` is true (the default), each handle (`ptr[i]`) will be freed.
 *       If `delete_targets` is false, only the array itself is deleted.
 * @note The `use_free` parameter controls whether to use `free()` (default, for C-allocated strings)
 *       or `delete[]` (for C++-allocated arrays) when deleting each handle.
 *
 * Example usage:
 * @code
 *   std::unique_ptr<char*[], HandleArrayDeleter> handles(
 *       Link::targets_to_handles(targets),
 *       HandleArrayDeleter(targets.size())
 *   );
 * @endcode
 */
struct HandleArrayDeleter {
    size_t size;
    bool delete_targets = true;  ///< If true, delete the target handles.
    bool use_free = true;        ///< If true, use free() instead of delete[] for handles.

    /**
     * @brief Construct a HandleArrayDeleter.
     * @param size The number of handles in the array.
     * @param delete_targets If true, delete each handle in the array as well as the array itself.
     * @param use_free If true, use free() instead of delete[] for handles.
     */
    HandleArrayDeleter(size_t size, bool delete_targets = true, bool use_free = true)
        : size(size), delete_targets(delete_targets), use_free(use_free) {}

    /**
     * @brief Function call operator to delete the array and optionally its contents.
     * @param ptr The array of C-string handles to delete.
     */
    void operator()(char** ptr) const {
        if (!ptr) return;
        if (delete_targets)
            for (size_t i = 0; i < size; ++i)
                if (ptr[i]) (use_free ? free(ptr[i]) : delete[] ptr[i]);
        delete[] ptr;
    }
};

// -------------------------------------------------------------------------------------------------
class Node : public Atom {
   public:
    string name;  ///< The name of the node.

    /**
     * @brief Construct a Node.
     * @param type The type of the node.
     * @param name The name of the node.
     * @param custom_attributes Custom attributes for the atom.
     * @throws std::runtime_error if type or name is empty.
     */
    Node(const string& type, const string& name, const Properties& custom_attributes = {})
        : Atom(type, custom_attributes), name(name) {
        this->validate();
    }

    /**
     * @brief Copy constructor.
     * Deeply copies node type, name and all custom attributes.
     *
     * @param other Node to be copied.
     */
    Node(const Node& other) : Atom(other) { this->name = other.name; }

    /**
     * @brief Assignment operator.
     * Deeply copies node type, name and all custom attributes.
     *
     * @param other Node to be copied.
     */
    virtual Node& operator=(const Node& other) {
        Atom::operator=(other);
        this->name = other.name;
        return *this;
    }

    /**
     * @brief Comparisson operator.
     * Deeply compare all custom attribute values in addition to node type and name.
     *
     * @param other Node to be compared to.
     */
    virtual bool operator==(const Node& other) {
        return (this->name == other.name) && Atom::operator==(other);
    }

    /**
     * @brief Comparisson operator.
     * Deeply compare all custom attribute values in addition to node type and name.
     *
     * @param other Node to be compared to.
     */
    virtual bool operator!=(const Node& other) { return !(*this == other); }

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
     * @brief Constructs and returns this Atom's handle.
     *
     * @return A newly built handle for this atom.
     */
    virtual string handle() const {
        char* handle_cstr = Node::compute_handle(this->type, this->name);
        string handle_string(handle_cstr);
        free(handle_cstr);
        return handle_string;
    }

    /**
     * @brief Constructs and returns a MeTTa expression which represents this Atom.
     *
     * @return a MeTTa expression which represents this Atom.
     */
    virtual string metta_representation(HandleDecoder& decoder) const {
        if (this->type != MettaMapping::SYMBOL_NODE_TYPE) {
            Utils::error("Can't compute metta expression of node whose type (" + this->type +
                         ") is not " + MettaMapping::SYMBOL_NODE_TYPE);
        }
        return this->name;
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
    vector<string> targets;  ///< The handles of the target atoms of the link.

    /**
     * @brief Construct a Link.
     * @param type The type of the link.
     * @param targets The target atoms of the link.
     * @param custom_attributes Custom attributes for the atom.
     * @throws std::runtime_error if type is empty or targets.size() < MINIMUM_TARGETS_SIZE.
     */
    Link(const string& type,
         const vector<const Atom*>& targets,
         const Properties& custom_attributes = {})
        : Atom(type, custom_attributes) {
        this->targets.reserve(targets.size());
        for (const Atom* atom : targets) {
            this->targets.push_back(atom->handle());
        }
        this->validate();
    }

    /**
     * @brief Construct a Link.
     * @param type The type of the link.
     * @param targets The target handles of the link.
     * @param custom_attributes Custom attributes for the atom.
     * @throws std::runtime_error if type is empty or targets.size() < MINIMUM_TARGETS_SIZE.
     */
    Link(const string& type, const vector<string>& targets, const Properties& custom_attributes = {})
        : Atom(type, custom_attributes), targets(targets) {
        this->validate();
    }

    /**
     * @brief Copy constructor.
     * Deeply copies link type, targets and all custom attributes.
     *
     * @param other Link to be copied.
     */
    Link(const Link& other) : Atom(other) { this->targets = other.targets; }

    /**
     * @brief Assignment operator.
     * Deeply copies link type, targets and all custom attributes.
     *
     * @param other Link to be copied.
     */
    virtual Link& operator=(const Link& other) {
        Atom::operator=(other);
        this->targets = other.targets;
        return *this;
    }

    /**
     * @brief Comparisson operator.
     * Deeply compare all custom attribute values in addition to link type and targets.
     *
     * @param other Link to be compared to.
     */
    virtual bool operator==(const Link& other) {
        return Atom::operator==(other) && (this->targets == other.targets);
    }

    /**
     * @brief Comparisson operator.
     * Deeply compare all custom attribute values in addition to link type and targets.
     *
     * @param other Link to be compared to.
     */
    virtual bool operator!=(const Link& other) { return !(*this == other); }

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

    virtual string to_string() const {
        string result = "Link(type: '" + this->type + "', targets: [";
        if (!this->targets.empty()) {
            for (const auto& target : this->targets) {
                result += target + ", ";
            }
            result.erase(result.size() - 2);  // Remove the last comma and space
        }
        result += "], custom_attributes: " + this->custom_attributes.to_string() + ")";
        return result;
    }

    /**
     * @brief Constructs and returns this Atom's handle.
     *
     * @return A newly built handle for this atom.
     */
    virtual string handle() const {
        char* handle_cstr = Link::compute_handle(this->type, this->targets);
        string handle_string(handle_cstr);
        free(handle_cstr);
        return handle_string;
    }

    /**
     * @brief Constructs and returns this Atom's composite type hash.
     *
     * @return A newly built composite type hash for this atom.
     */
    virtual string composite_type_hash(HandleDecoder& decoder) const {
        unsigned int arity = this->targets.size();
        unsigned int cursor = 0;
        char** handles = new char*[arity + 1];
        for (string handle : this->composite_type(decoder)) {
            handles[cursor++] = strdup(handle.c_str());
        }
        char* hash_cstr = ::composite_hash(handles, arity + 1);

        string hash_string(hash_cstr);
        free(hash_cstr);
        for (int i = arity; i >= 0; i--) {
            free(handles[i]);
        }
        delete[] handles;
        return hash_string;
    }

    /**
     * @brief Constructs and returns this Atom's composite type hash.
     *
     * @return A newly built composite type for this atom.
     */
    virtual vector<string> composite_type(HandleDecoder& decoder) const {
        vector<string> composite_type;
        composite_type.push_back(named_type_hash());
        for (string handle : this->targets) {
            shared_ptr<Atom> atom = decoder.get_atom(handle);
            if (atom == NULL) {
                Utils::error("Unkown atom with handle: " + handle);
                return {};
            } else {
                composite_type.push_back(atom->composite_type_hash(decoder));
            }
        }
        return composite_type;
    }

    /**
     * @brief Constructs and returns a MeTTa expression which represents this Atom.
     *
     * @return a MeTTa expression which represents this Atom.
     */
    virtual string metta_representation(HandleDecoder& decoder) const {
        if (this->type != MettaMapping::EXPRESSION_LINK_TYPE) {
            Utils::error("Can't compute metta expression of link whose type (" + this->type +
                         ") is not " + MettaMapping::EXPRESSION_LINK_TYPE);
        }
        string metta_string = "(";
        unsigned int size = this->targets.size();
        for (unsigned int i = 0; i < size; i++) {
            metta_string += decoder.get_atom(this->targets[i])->metta_representation(decoder);
            if (i != (size - 1)) {
                metta_string += " ";
            }
        }
        metta_string += ")";
        return metta_string;
    }

    /**
     * @brief Return this Link's arity
     * @return this Link's arity.
     */
    unsigned int arity() const { return this->targets.size(); }

    /**
     * @brief Convert a vector of Atom pointers to an array of handles.
     *
     * For each Atom in the input vector, computes its handle (allocating a new C-string for each).
     *
     * @param targets The vector of Atom pointers.
     * @return A newly allocated array of C-string handles (char**). Caller is responsible for
     * freeing each handle with free() and the array itself with delete[].
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
        unique_ptr<char*[], HandleArrayDeleter> targets_handles(Link::targets_to_handles(targets),
                                                                HandleArrayDeleter(targets.size()));

        return Link::compute_handle(type.c_str(), targets_handles.get(), targets.size());
    }

    /**
     * @brief Compute the handle for a Link given its type and targets (string handles).
     *
     * This static utility computes a handle for a Link using the provided type and a vector of
     * target handles (as strings), without requiring an instantiated Link object.
     *
     * @param type The type of the link.
     * @param targets The handles of the target atoms as strings.
     * @return A newly allocated char array containing the handle (caller must free).
     * @note Caller is responsible for freeing the returned handle.
     */
    static char* compute_handle(const string& type, const vector<string>& targets) {
        unique_ptr<char*[], HandleArrayDeleter> targets_handles(
            new char*[targets.size()], HandleArrayDeleter(targets.size(), false));
        size_t i = 0;
        for (const auto& target : targets) targets_handles[i++] = (char*) target.c_str();
        return Link::compute_handle(type.c_str(), targets_handles.get(), targets.size());
    }

    /**
     * @brief Compute the handle for a Link given its type hash and target handles.
     *
     * This private static utility computes a handle for a Link using the provided type hash and an
     * array of target handles. Used internally by the public compute_handle methods.
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
        unique_ptr<char, HandleDeleter> type_hash(::named_type_hash((char*) type));
        return expression_hash(type_hash.get(), targets_handles, targets_size);
    }
};

}  // namespace atomspace
