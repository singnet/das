#pragma once

#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <stack>

#include "Atom.h"
#include "Node.h"
#include "HandleDecoder.h"
#include "MettaMapping.h"
#include "Properties.h"
#include "Hasher.h"

#define HANDLE_SIZE ((unsigned int) 32)
#define MINIMUM_TARGETS_SIZE ((unsigned int) 1)

using namespace std;
using namespace commons;
using namespace atoms;

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
// -------------------------------------------------------------------------------------------------
class Link : public Atom {
   public:
    vector<string> targets;  ///< The handles of the target atoms of the link.

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
        if (this->type == Atom::UNDEFINED_TYPE) {
            Utils::error("Link type can't be '" + Atom::UNDEFINED_TYPE + "'");
        }
        if (this->targets.size() < MINIMUM_TARGETS_SIZE) {
            Utils::error("Link must have at least " + std::to_string(MINIMUM_TARGETS_SIZE) + " targets");
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
        return Hasher::link_handle(this->type, this->targets);
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

// XXXXX    /**
// XXXXX     * @brief Convert a vector of Atom pointers to an array of handles.
// XXXXX     *
// XXXXX     * For each Atom in the input vector, computes its handle (allocating a new C-string for each).
// XXXXX     *
// XXXXX     * @param targets The vector of Atom pointers.
// XXXXX     * @return A newly allocated array of C-string handles (char**). Caller is responsible for
// XXXXX     * freeing each handle with free() and the array itself with delete[].
// XXXXX     * @throws std::runtime_error if a target is not a Node or Link.
// XXXXX     */
// XXXXX    static char** targets_to_handles(const vector<const Atom*>& targets) {
// XXXXX        //-------------------------
// XXXXX        // TODO REMOVE THIS METHOD
// XXXXX        //-------------------------
// XXXXX        char** handles = new char*[targets.size()];  // Will be freed later by the caller
// XXXXX        size_t i = 0;
// XXXXX        for (const auto& target : targets) {
// XXXXX            if (const Node* node = dynamic_cast<const Node*>(target)) {
// XXXXX                handles[i++] = (char*) Hasher::node_handle(node->type, node->name).c_str();
// XXXXX            } else if (const Link* link = dynamic_cast<const Link*>(target)) {
// XXXXX                handles[i++] = Link::compute_handle(link->type, link->targets);
// XXXXX            } else {
// XXXXX                Utils::error("Unsupported target type in Link::targets_to_handles");
// XXXXX            }
// XXXXX        }
// XXXXX        return handles;
// XXXXX    }

};

// -------------------------------------------------------------------------------------------------
class Wildcard : public Atom {
   public:

    /**
     * @brief Construct a Wildcard.
     * @param type The type of the atom.
     * @throws std::runtime_error if type is empty.
     */
    Wildcard(const string& type, const Properties& custom_attributes = {}) : Atom(type, custom_attributes) {
        this->validate();
    }

    /**
     * @brief Copy constructor.
     *
     * @param other Wildcard to be copied.
     */
    Wildcard(const Wildcard& other) : Atom(other) {}

    /**
     * @brief Assignment operator.
     *
     * @param other Wildcard to be copied.
     */
    virtual Wildcard& operator=(const Wildcard& other) {
        Atom::operator=(other);
        return *this;
    }

    /**
     * @brief Comparisson operator.
     *
     * @param other Wildcard to be compared to.
     */
    virtual bool operator==(const Wildcard& other) {
        return  Atom::operator==(other);
    }

    /**
     * @brief Comparisson operator.
     *
     * @param other Wildcard to be compared to.
     */
    virtual bool operator!=(const Wildcard& other) { return !(*this == other); }

    /**
     * @brief Validate the Wildcard object.
     */
    virtual void validate() const override {
        Atom::validate();  // Validate type
    }

    virtual string to_string() const {
        return "Wildcard(type: '" + this->type + "')";
    }

    virtual string handle() const = 0;

    /**
     * @brief returns the handle this Atom use when inserted into a schema.
     *
     * @return schema handle for this atom.
     */
    virtual string schema_handle() const {
        return Atom::WILDCARD_HANDLE;
    }

    virtual string metta_representation(HandleDecoder& decoder) const = 0;
};

// -------------------------------------------------------------------------------------------------
class UntypedVariable : public Wildcard {
   public:
    string name;  ///< The name of the variable.

    /**
     * @brief Construct a UntypedVariable.
     * @param name The name of the node.
     * @throws std::runtime_error if name is empty.
     */
    UntypedVariable(const string& name) : Wildcard(Atom::UNDEFINED_TYPE), name(name) {
        this->validate();
    }

    /**
     * @brief Copy constructor.
     * Deeply copies node type, name
     *
     * @param other UntypedVariable to be copied.
     */
    UntypedVariable(const UntypedVariable& other) : Wildcard(other) { this->name = other.name; }

    /**
     * @brief Assignment operator.
     * Deeply copies node type, name
     *
     * @param other UntypedVariable to be copied.
     */
    virtual UntypedVariable& operator=(const UntypedVariable& other) {
        Wildcard::operator=(other);
        this->name = other.name;
        return *this;
    }

    /**
     * @brief Comparisson operator.
     *
     * @param other UntypedVariable to be compared to.
     */
    virtual bool operator==(const UntypedVariable& other) {
        return (this->name == other.name) && Wildcard::operator==(other);
    }

    /**
     * @brief Comparisson operator.
     *
     * @param other UntypedVariable to be compared to.
     */
    virtual bool operator!=(const UntypedVariable& other) { return !(*this == other); }

    /**
     * @brief Validate the UntypedVariable object.
     *
     * Checks that the type is valid (by calling Atom::validate()) and that the name is not empty.
     *
     * @throws std::runtime_error if the name is empty.
     */
    virtual void validate() const override {
        if (this->type != Atom::UNDEFINED_TYPE) {
            Utils::error("Invalid type for UntypedVariable: " + this->type + " (expected " + Atom::UNDEFINED_TYPE + ")");
        }
        if (this->name.empty()) {
            Utils::error("Invalid empty name for UntypedVariable");
        }
    }

    virtual string to_string() const {
        return "UntypedVariable(name: '" + this->name + "')";
    }

    /**
     * @brief Constructs and returns this Atom's handle.
     *
     * @return A newly built handle for this atom.
     */
    virtual string handle() const {
        return Hasher::node_handle(this->type, this->name);
    }

    /**
     * @brief Constructs and returns a MeTTa expression which represents this Atom.
     *
     * @return a MeTTa expression which represents this Atom.
     */
    virtual string metta_representation(HandleDecoder& decoder) const {
        return "$" + this->name;
    }
};

// -------------------------------------------------------------------------------------------------
class LinkSchema : public Wildcard {
   private:

    bool _frozen;
    unsigned int _arity;
    vector<string> _schema;
    vector<string> _composite_type;
    string _composite_type_hash;
    string _atom_handle;
    string _metta_representation;
    stack<tuple<string, string, string>> _atom_stack;

   public:

    /**
     * @brief Construct a LinkSchema.
     * @param type The type of the link.
     * @param LinkSchema arity
     * @param custom_attributes Custom attributes for the schema.
     */
    LinkSchema(const string& type, unsigned int arity, const Properties& custom_attributes = {}) : Wildcard(type, custom_attributes) {
        this->_frozen = false;
        this->_arity = arity;
        this->_schema.reserve(arity);
        this->_composite_type.reserve(arity + 1);
        this->_composite_type.push_back(named_type_hash());
        this->_metta_representation = "(";
    }

    void add_target(const string& schema_handle, const string& composite_type_hash, const string& metta_representation) {
        if (! this->_frozen) {
            this->_schema.push_back(schema_handle);
            this->_composite_type.push_back(composite_type_hash);
            this->_metta_representation += metta_representation;
            if (this->_schema.size() == _arity) {
                this->_frozen = true;
                this->_metta_representation += ")";
                this->_atom_handle = Hasher::link_handle(type, this->_schema);
                this->_composite_type_hash = Hasher::composite_handle(this->_composite_type);
                this->validate();
            } else {
                this->_metta_representation += " ";
            }
        } else {
            Utils::error("Attempt to add a new target beyond LinkSchema's arity. LinkSchema: " + this->to_string());
        }
    }

    void stack_node(const string& type, const string& name) {
        if (! check_not_frozen()) {
            tuple<string, string, string> triplet(Hasher::node_handle(type, name), Hasher::type_handle(type), name);
            this->_atom_stack.push(triplet);
        }
    }

    void stack_untyped_variable(const string& name) {
        if (! check_not_frozen()) {
            UntypedVariable variable(name);
            tuple<string, string, string> triplet(variable.schema_handle(), variable.named_type_hash(), "$" + name);
            this->_atom_stack.push(triplet);
        }
    }

    void stack_link(const string& type, unsigned int link_arity) {
        if (check_not_frozen()) {
            return;
        }
        if (this->_atom_stack.size() >= link_arity) {
            vector<string> target_handles;
            vector<string> composite_type;
            string metta_expression = "(";
            target_handles.reserve(link_arity);
            composite_type.reserve(link_arity);
            for (int i = link_arity - 1; i >= 0; i--) {
                tuple<string, string, string> triplet = this->_atom_stack.top();
                target_handles[i] = get<0>(triplet);
                composite_type[i] = get<1>(triplet);
                metta_expression += get<2>(triplet);
                if (i != 0) {
                    metta_expression += " ";
                }
                this->_atom_stack.pop();
            }
            metta_expression += ")";
            tuple<string, string, string> triplet(Hasher::link_handle(type, target_handles), Hasher::composite_handle(composite_type), metta_expression);
            this->_atom_stack.push(triplet);
        } else {
            Utils::error("Couldn't stack link. Link arity: " + std::to_string(link_arity) + " stack size: " + std::to_string(this->_atom_stack.size()));
        }
    }

    void build() {
        if (! check_not_frozen()) {
            if (this->_atom_stack.size() == this->_arity) {
               for (unsigned int i = 0; i < this->_arity; i++) {
                   tuple<string, string, string> triplet = this->_atom_stack.top();
                   add_target(get<0>(triplet), get<1>(triplet), get<2>(triplet));
                   this->_atom_stack.pop();
               }
            } else {
                Utils::error("Can't build LinkTemplate of arity " + std::to_string(this->_arity) + " out of a stack with " + std::to_string(this->_atom_stack.size()) + " atoms.");
            }
        }
    }

    /**
     * @brief Copy constructor.
     * Deeply copies link type, targets and all custom attributes.
     *
     * @param other LinkSchema to be copied.
     */
    LinkSchema(const LinkSchema& other) : Wildcard(other) {
        *this = other;
    }

    /**
     * @brief Assignment operator.
     * Deeply copies link type, targets and all custom attributes.
     *
     * @param other LinkSchema to be copied.
     */
    virtual LinkSchema& operator=(const LinkSchema& other) {
        if (! other._frozen) {
            Utils::error("Can't clone an unfrozen LinkTemplate");
        }
        Wildcard::operator=(other);
        this->_arity = other._arity;
        this->_frozen = other._frozen;
        this->_schema = other._schema;
        this->_composite_type = other._composite_type;
        this->_composite_type_hash = other._composite_type_hash;
        this->_atom_handle = other._atom_handle;
        this->_metta_representation = other._metta_representation;
        return *this;
    }

    /**
     * @brief Comparisson operator.
     *
     * @param other LinkSchema to be compared to.
     */
    virtual bool operator==(const LinkSchema& other) {
        return this->_atom_handle == other._atom_handle;
    }

    /**
     * @brief Comparisson operator.
     *
     * @param other LinkSchema to be compared to.
     */
    virtual bool operator!=(const LinkSchema& other) { return !(*this == other); }

    /**
     * @brief Validate the LinkSchema object.
     *
     * @throws std::runtime_error if the number of targets is less than MINIMUM_TARGETS_SIZE.
     */
    void validate() const {
        if (this->type == Atom::UNDEFINED_TYPE) {
            Utils::error("LinkSchema type can't be '" + Atom::UNDEFINED_TYPE + "'");
        }
        if (this->_schema.size() < MINIMUM_TARGETS_SIZE) {
            Utils::error("LinkSchema must have at least " + std::to_string(MINIMUM_TARGETS_SIZE) + " targets");
        }
        bool flag = true;
        for (string schema_handle: this->_schema) {
            if (schema_handle == Atom::WILDCARD_HANDLE) {
                flag = false;
                break;
            }
        }
        if (flag) {
            Utils::error("Invalid LinkSchema with no variables and no nested link schemas");
        }
    }

    virtual string to_string() const {
        if (!this->_frozen) {
            Utils::error("Can't call to_string() before finishing LinkTemplate by calling build()");
        }
        string result = "LinkSchema(type: '" + this->type + "', targets: [";
        if (!this->_schema.empty()) {
            for (const auto& target : this->_schema) {
                result += target + ", ";
            }
            result.erase(result.size() - 2);  // Remove the last comma and space
        }
        result += "], custom_attributes: " + this->custom_attributes.to_string() + ")";
        return result;
    }

    /**
     * @brief Returns this Atom's handle.
     *
     * @return Handle for this atom.
     */
    virtual string handle() const {
        return this->_atom_handle;
    }

    /**
     * @brief Returns this Atom's composite type hash.
     *
     * @return composite type hash for this atom.
     */
    virtual string composite_type_hash(HandleDecoder& decoder) const {
        return this->_composite_type_hash;
    }

    /**
     * @brief Returns this Atom's composite type hash.
     *
     * @return composite type for this atom.
     */
    virtual vector<string> composite_type(HandleDecoder& decoder) const {
        return this->_composite_type;
    }

    /**
     * @brief Constructs and returns a MeTTa expression which represents this Atom.
     *
     * @return a MeTTa expression which represents this Atom.
     */
    virtual string metta_representation(HandleDecoder& decoder) const {
        return this->_metta_representation;
    }

    /**
     * @brief Return this LinkSchema's arity
     * @return this LinkSchema's arity.
     */
    unsigned int arity() const { return this->_arity; }

    bool check_not_frozen() {
        if (this->_frozen) {
            Utils::error("Can't change a LinkTemplate after calling build()");
            return true;
        } else {
            return false;
        }
    }
};

}  // namespace atomspace
