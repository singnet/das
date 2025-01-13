/**
 * @file document_types.h
 * @brief Defines various classes representing atomic entities and their relationships in the atom
 * database.
 *
 * This header file contains the definitions of several classes that represent different types of
 * atomic entities and their relationships within the atom database. The classes include:
 * - CustomAttributes: A type alias for `std::unordered_map<string, variant<string, long, double, bool>`.
 * - Atom: Represents a basic atomic entity with attributes such as ID, handle, composite type hash,
 *   named type, and optional custom attributes.
 * - Node: Represents a node in the atom database, extending the Atom class by adding a name
 *   attribute.
 * - Link: Represents a link in the atom database, encapsulating a composite type, named type hash,
 *   and a list of target hashes. It supports nested composite types and validates their structure.
 */
#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <variant>

#include "type_aliases.h"

using namespace std;

namespace atomdb {

using CustomAttributesKey = string;
using CustomAttributesValue = variant<string, long, double, bool>;
using CustomAttributes = unordered_map<CustomAttributesKey, CustomAttributesValue>;

/**
 * @brief Retrieves a custom attribute from the given custom attributes.
 * @param custom_attributes The custom attributes map.
 * @param key The key for the custom attribute to retrieve.
 * @return An optional value of type T if the custom attribute exists.
 */
template <typename T>
static const optional<T> get_custom_attribute(const CustomAttributes& custom_attributes,
                                              const CustomAttributesKey& key) {
    if (custom_attributes.find(key) != custom_attributes.end()) {
        return std::get<T>(custom_attributes.at(key));
    }
    return nullopt;
}

/**
 * @brief Converts custom attributes to a string representation.
 * @param custom_attributes The custom attributes to be converted.
 * @return A string representation of the custom attributes.
 */
static string custom_attributes_to_string(const CustomAttributes& custom_attributes) {
    if (custom_attributes.empty()) {
        return "{}";
    }
    string result = "{";
    for (const auto& [key, value] : custom_attributes) {
        result += key + ": ";
        if (auto str = std::get_if<string>(&value)) {
            result += "'" + *str + "'";
        } else if (auto integer = std::get_if<long>(&value)) {
            result += std::to_string(*integer);
        } else if (auto floating = std::get_if<double>(&value)) {
            result += std::to_string(*floating);
        } else if (auto boolean = std::get_if<bool>(&value)) {
            result += *boolean ? "true" : "false";
        }
        result += ", ";
    }
    result.pop_back();
    result.pop_back();
    result += "}";
    return move(result);
}

/**
 * @class Atom
 * @brief Represents an atomic entity with various attributes.
 *
 * The Atom class encapsulates the properties of an atomic entity, including its ID, handle,
 * composite type hash, named type, and optional custom attributes. It provides constructors
 * for initialization, comparison operators, and a method to convert the object to a string
 * representation.
 */
class Atom {
   public:
    string _id;
    string handle;
    string composite_type_hash;
    string named_type;
    CustomAttributes custom_attributes;

    Atom() = default;

    /**
     * @brief Constructs an Atom with a named type and optional custom attributes.
     * @param named_type The named type of the Atom.
     * @param custom_attributes Optional custom attributes for the Atom. Defaults to an empty map.
     * @note This constructor is intended to be used only when passing in the basic building
     *       parameters to other functions. For creating complete new Atom objects, use the
     *       constructor with all parameters.
     */
    Atom(const string& named_type, const CustomAttributes& custom_attributes = CustomAttributes{})
        : named_type(named_type), custom_attributes(custom_attributes) {}

    /**
     * @brief Constructs an Atom object with the given parameters.
     * @param id The unique identifier for the atom.
     * @param handle The handle for the atom.
     * @param composite_type_hash The hash representing the composite type of the atom.
     * @param named_type The named type of the atom.
     * @param custom_attributes Optional custom attributes for the atom. Defaults to an empty map.
     */
    Atom(const string& id,
         const string& handle,
         const string& composite_type_hash,
         const string& named_type,
         const CustomAttributes& custom_attributes = CustomAttributes{})
        : _id(id),
          handle(handle),
          composite_type_hash(composite_type_hash),
          named_type(named_type),
          custom_attributes(custom_attributes) {}

    /**
     * @brief Validates the attributes of the object.
     * @throws std::invalid_argument if any attribute is invalid.
     */
    virtual void validate() const {
        if (this->_id.empty()) {
            throw invalid_argument("Atom ID cannot be empty.");
        }
        if (this->handle.empty()) {
            throw invalid_argument("Atom handle cannot be empty.");
        }
        if (this->composite_type_hash.empty()) {
            throw invalid_argument("Composite type hash cannot be empty.");
        }
        if (this->named_type.empty()) {
            throw invalid_argument("Named type cannot be empty.");
        }
    }

    virtual ~Atom() = default;

    /**
     * @brief Converts the object to a string representation.
     * @return A string representing the object, including its ID, handle, composite type hash,
     *         named type, and custom attributes.
     */
    virtual const string to_string() const noexcept {
        string result = "_id: '" + this->_id + "'";
        result += ", handle: '" + this->handle + "'";
        result += ", composite_type_hash: '" + this->composite_type_hash + "'";
        result += ", named_type: '" + this->named_type + "'";
        result += ", custom_attributes: ";
        result += custom_attributes_to_string(this->custom_attributes);
        return move(result);
    }
};

/**
 * @class Node
 * @brief Represents a node in the atom database, inheriting from Atom.
 *
 * The Node class extends the Atom class by adding a name attribute. It includes constructors,
 * equality operators, and a string representation method. The name attribute must not be empty.
 */
class Node : public Atom {
   public:
    string name;

    Node() = default;

    /**
     * @brief Constructs a Node with a type, name, and optional custom attributes.
     * @param type The type of the Node.
     * @param name The name of the Node.
     * @param custom_attributes Optional custom attributes for the Node. Defaults to an empty map.
     * @note This constructor is intended to be used only when passing in the basic building
     *       parameters to other functions. For creating complete new Node objects, use the
     *       constructor with all parameters.
     *
     * Usage:
     * ```
     * auto db = AtomDB();
     * shared_ptr<Node> node1 = db.add_node(Node("Concept", "monkey"));
     * shared_ptr<Node> node2 = db.add_node(
     *     Node(
     *         "Concept",                                 // type
     *         "human",                                   // name
     *         { {"weight": 0.8}, {"immutable": false} }  // custom_attributes (optional)
     *     )
     * );
     *
     * bool node2_is_immutable = (
     *     get_custom_attribute<bool>(node2->custom_attributes, "immutable").value_or(false)
     * );
     * cout << node2_is_immutable << endl;
     * // Output:
     * 0
     *
     * cout << node2.to_string() << endl;
     * // Output:
     * Node(_id: 'af12f10f9ae2002a1607ba0b47ba8407', handle: 'af12f10f9ae2002a1607ba0b47ba8407',
     * composite_type_hash: 'd99a604c79ce3c2e76a2f43488d5d4c3', named_type: 'Concept', custom_attributes:
     * {immutable: false, weight: 0.800000}, name: 'human')
     * ```
     */
    Node(const string& type,
         const string& name,
         const CustomAttributes& custom_attributes = CustomAttributes{})
        : name(name), Atom(type, custom_attributes) {}

    /**
     * @brief Constructs a Node object with the given parameters.
     * @param id The identifier for the Node.
     * @param handle The handle for the Node.
     * @param composite_type_hash The hash representing the composite type.
     * @param named_type The named type of the Node.
     * @param name The name of the Node.
     * @param custom_attributes Optional custom attributes for the Node. Defaults to an empty map.
     */
    Node(const string& id,
         const string& handle,
         const string& composite_type_hash,
         const string& named_type,
         const string& name,
         const CustomAttributes& custom_attributes = CustomAttributes{})
        : name(name), Atom(id, handle, composite_type_hash, named_type, custom_attributes) {}

    /**
     * @brief Validates the attributes of the object.
     * @throws std::invalid_argument if any attribute is invalid.
     */
    void validate() const override {
        Atom::validate();
        if (this->name.empty()) {
            throw invalid_argument("Node name cannot be empty.");
        }
    }

    /**
     * @brief Converts the Node object to a string representation.
     * @return A string representing the Node object.
     */
    const string to_string() const noexcept override {
        string result = "Node(" + Atom::to_string();
        result += ", name: '" + this->name + "')";
        return move(result);
    }
};

/**
 * @class Link
 * @brief Represents a link in the atom database, inheriting from Atom.
 *
 * The Link class encapsulates a composite type, a named type hash, and a list of target hashes.
 * It supports nested composite types and validates their structure. The class also provides
 * functionality to compare links, convert them to string representations, and manage target
 * documents.
 */
class Link : public Atom {
   public:
    using TargetsDocuments = vector<variant<Node, Link>>;

    /**
     * `composite_type` is designed to hold a list of elements, where each element can either be a
     * `string` (single hash) or another list of `strings`, allowing multiple levels of nesting.
     * Example:
     ```
       [
          "5b19fd94cb294ecebae572045771f1ac",
          "f74c835e804a481c8918e16671cabcde",
          [
             "5df522aeaaf04617b96128bd5ca7ad17",
             [
                "0a9aada9568e48febfc4395657a82840",
                "72645c1ec2ef46d88542d54484cb45e9"
             ],
             "d3d4832042af4fdab5983c63a66885d2"
          ]
       ]
     ```
     */
    ListOfAny composite_type;

    string named_type_hash;
    vector<string> targets;
    bool is_toplevel = true;
    TargetsDocuments targets_documents;

    Link() = default;

    /**
     * @brief Constructs a Link with a type, targets documents, and optional custom attributes.
     * @param type The type of the Link.
     * @param targets The targets documents associated with the Link.
     * @param custom_attributes Optional custom attributes for the Link. Defaults to an empty map.
     * @note This constructor is intended to be used only when passing in the basic building
     *       parameters to other functions. For creating complete new Link objects, use the
     *       constructor with all parameters.
     *
     * Usage:
     * ```
     * auto db = AtomDB();
     * auto link = db.add_link(
     *     Link(
     *         "Similarity",                                 // type
     *         {                                             // targets of Similarity link
     *             {Node("Concept", "monkey")},              // a node as a target of Similarity link
     *             {Node("Concept", "human")},               // another node as a target
     *             {                                         // a link as a target of Similarity link
     *                 Link("Dummicity",                     // type
     *                      {                                // targets of Dummicity link
     *                          {Node("Concept", "dummy1")},
     *                          {Node("Concept", "dummy2")}
     *                      }
     *                 )
     *             }
     *         },
     *         { {"weight": 0.8}, {"immutable": false} }     // custom_attributes (optional)
     *     )
     * );
     *
     * double link_weight = (
     *     get_custom_attribute<double>(link->custom_attributes, "weight").value_or(0.0)
     * );
     * cout << link_weight << endl;
     * // Output:
     * 0.800000
     *
     * cout << link.to_string() << endl;
     * // Output:
     * Link(_id: '8ef9c2093150a022204fa6c9f0bc94f8', handle: '8ef9c2093150a022204fa6c9f0bc94f8',
     * composite_type_hash: '8dd4c9ab591dbbd1e6eb511e71ef5aa9', named_type: 'Similarity',
     * custom_attributes: {immutable: false, weight: 0.800000}, composite_type:
     * ['a9dea78180588431ec64d6bc4872fdbc', 'd99a604c79ce3c2e76a2f43488d5d4c3',
     * 'd99a604c79ce3c2e76a2f43488d5d4c3', ['44d25cf84a95f144d6e603ca28caadba',
     * 'd99a604c79ce3c2e76a2f43488d5d4c3', 'd99a604c79ce3c2e76a2f43488d5d4c3']], named_type_hash:
     * 'a9dea78180588431ec64d6bc4872fdbc', targets: ['1cdffc6b0b89ff41d68bec237481d1e1',
     * 'af12f10f9ae2002a1607ba0b47ba8407', '087091095a266df1bfcc1ee01e79811c'], is_toplevel: true,
     * targets_documents: [])
     * ```
     */
    Link(const string& type,
         const TargetsDocuments& targets,
         const CustomAttributes& custom_attributes = CustomAttributes{})
        : targets_documents(targets), Atom(type, custom_attributes) {}

    /**
     * @brief Constructs a Link object with the specified parameters.
     * @param id The identifier for the link.
     * @param handle The handle for the link.
     * @param composite_type_hash The hash of the composite type.
     * @param named_type The named type of the link.
     * @param composite_type The composite type list.
     * @param named_type_hash The hash of the named type.
     * @param targets The vector of target strings.
     * @param is_toplevel Boolean indicating if the link is top-level.
     * @param custom_attributes Optional custom attributes. Defaults to an empty map.
     * @param targets_documents Optional targets documents. Defaults to an empty vector.
     */
    Link(const string& id,
         const string& handle,
         const string& composite_type_hash,
         const string& named_type,
         const ListOfAny& composite_type,
         const string& named_type_hash,
         const vector<string>& targets,
         bool is_toplevel,
         const CustomAttributes& custom_attributes = CustomAttributes{},
         const TargetsDocuments& targets_documents = TargetsDocuments{})
        : composite_type(composite_type),
          named_type_hash(named_type_hash),
          targets(targets),
          is_toplevel(is_toplevel),
          targets_documents(targets_documents),
          Atom(id, handle, composite_type_hash, named_type, custom_attributes) {}

    /**
     * @brief Validates the attributes of the object.
     * @throws std::invalid_argument if any attribute is invalid.
     */
    void validate() const override {
        Atom::validate();
        if (this->composite_type.empty()) {
            throw invalid_argument("Composite type cannot be empty.");
        }
        if (not this->validate_composite_type(this->composite_type)) {
            throw invalid_argument(
                "Invalid composite type. All elements must be strings or lists of strings.");
        }
        if (this->named_type_hash.empty()) {
            throw invalid_argument("Named type hash cannot be empty.");
        }
        if (this->targets.empty()) {
            throw invalid_argument("Link targets cannot be empty.");
        }
    }

    /**
     * @brief Converts the Link object to a string representation.
     * @return A string representing the Link object, including its composite type, named type hash,
     *         targets, top-level status, and target documents.
     */
    const string to_string() const noexcept override {
        string result = "Link(" + Atom::to_string();
        result += ", composite_type: " + composite_type_list_to_string(this->composite_type);
        result += ", named_type_hash: '" + this->named_type_hash + "'";
        result += ", targets: [";
        if (not this->targets.empty()) {
            for (const auto& target : this->targets) {
                result += "'" + target + "', ";
            }
            result.pop_back();
            result.pop_back();
        }
        result += "]";  // end of targets
        result += ", is_toplevel: ";
        result += this->is_toplevel ? "true" : "false";
        result += ", targets_documents: [";
        if (not this->targets_documents.empty()) {
            for (const auto& target : this->targets_documents) {
                if (auto node = std::get_if<Node>(&target)) {
                    result += string(node->to_string()) + ", ";
                } else if (auto link = std::get_if<Link>(&target)) {
                    result += string(link->to_string()) + ", ";
                }
            }
            result.pop_back();
            result.pop_back();
        }
        result += "]";  // end of targets_documents
        result += ")";  // end of Link
        return move(result);
    }

    /**
     * @brief Converts a composite type list to a string representation.
     * @param composite_type The list of any type elements to be converted.
     * @return A string representation of the composite type list.
     */
    const string composite_type_list_to_string(const ListOfAny& composite_type) const noexcept {
        string result = "[";
        if (not composite_type.empty()) {
            for (const auto& element : composite_type) {
                if (auto str = any_cast<string>(&element)) {
                    result += "'" + *str + "', ";
                } else if (auto list = any_cast<ListOfAny>(&element)) {
                    result += composite_type_list_to_string(*list) + ", ";
                }
            }
            result.pop_back();
            result.pop_back();
        }
        result += "]";
        return move(result);
    }

   private:
    /**
     * @brief Validates the structure of a composite type.
     *
     * This function checks whether the given composite type adheres to the expected structure.
     * A composite type is a list where each element can be either a string or another list of
     * the same type. The function ensures that all elements in the composite type meet these
     * criteria.
     *
     * @param composite_type A list of elements of type std::any representing the composite type
     *                       to be validated.
     * @return true if the composite type is valid, false otherwise.
     */
    static bool validate_composite_type(const ListOfAny& composite_type) {
        for (const auto& element : composite_type) {
            if (auto str = any_cast<string>(&element)) {
                continue;
            } else if (auto list = any_cast<ListOfAny>(&element)) {
                if (not Link::validate_composite_type(*list)) {
                    return false;
                }
            } else {
                return false;
            }
        }
        return true;
    }
};

using AtomList = vector<Atom>;
using NodeList = vector<Node>;
using LinkList = vector<Link>;

}  // namespace atomdb
