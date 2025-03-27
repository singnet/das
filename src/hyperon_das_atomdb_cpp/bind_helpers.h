/**
 * @file bind_helpers.h
 * @brief This header file contains helper functions and type definitions for binding C++
 *        classes and structures to Python using the `nanobind` library. The primary purpose
 *        of these helpers is to facilitate the conversion of C++ objects to Python objects
 *        and vice versa, enabling seamless interoperability between the two languages.
 *
 * The file includes:
 * - Type definitions for tuples representing various C++ structures.
 * - Functions for converting between C++ and Python representations of these structures.
 * - Functions for converting composite types between C++ lists and Python lists.
 * - Functions for converting C++ objects to Python dictionaries.
 * - Functions for initializing and updating C++ objects from Python representations.
 */
#pragma once

#include <database.h>
#include <document_types.h>
#include <nanobind/nanobind.h>
#include <type_aliases.h>

using namespace std;
using namespace atomdb;

namespace nb = nanobind;

namespace bind_helpers {

// Tuples for `pickle` conversion between C++ and Python
// See `__getstate__` and `__setstate__` in the bindings.

using NodeTuple =                // Tuple for Node
    std::tuple<string,           // _id
               string,           // handle
               string,           // composite_type_hash
               string,           // named_type
               string,           // name
               CustomAttributes  // custom_attributes
               >;

using LinkTuple =                      // Tuple for Link
    std::tuple<string,                 // _id
               string,                 // handle
               string,                 // composite_type_hash
               string,                 // named_type
               nb::list,               // composite_type
               string,                 // named_type_hash
               vector<string>,         // targets
               bool,                   // is_toplevel
               CustomAttributes,       // custom_attributes
               Link::TargetsDocuments  // targets_documents
               >;

/**
 * @brief Converts a composite type list to a Python list.
 * @param ct_list The composite type list to be converted.
 * @return A Python list (`nb::list`) containing the elements of the composite type list.
 */
static nb::list composite_type_to_pylist(const ListOfAny& ct_list) {
    nb::list py_list;
    for (const auto& element : ct_list) {
        if (auto str = any_cast<string>(&element)) {
            py_list.append(*str);
        } else if (auto list = any_cast<ListOfAny>(&element)) {
            py_list.append(composite_type_to_pylist(*list));
        } else {
            throw invalid_argument("Invalid composite type element.");
        }
    }
    return move(py_list);
}

/**
 * @brief Converts a Python list to a composite type list.
 * @param py_list The Python list to be converted.
 * @return A composite type list (`ListOfAny`) containing the elements of the Python list.
 */
static ListOfAny pylist_to_composite_type(const nb::list& py_list) {
    ListOfAny ct_list;
    ct_list.reserve(py_list.size());
    for (const auto& element : py_list) {
        const auto& e_type = element.type();
        auto type_name = string(nb::type_name(e_type).c_str());
        if (type_name == "str") {
            ct_list.push_back(move(nb::cast<string>(element)));
        } else if (type_name == "list") {
            ct_list.push_back(move(pylist_to_composite_type(nb::cast<nb::list>(element))));
        } else {
            throw invalid_argument("Invalid composite type element.");
        }
    }
    return move(ct_list);
}

/**
 * @brief Converts an Atom object to a Python dictionary.
 * @param self The Atom object to be converted.
 * @return A Python dictionary (`nb::dict`) containing the attributes of the Atom.
 */
static nb::dict atom_to_dict(const Atom& self) {
    nb::dict dict;
    dict[FieldNames::ID_HASH] = self._id;
    dict[FieldNames::HANDLE] = self.handle;
    dict[FieldNames::COMPOSITE_TYPE_HASH] = self.composite_type_hash;
    dict[FieldNames::TYPE_NAME] = self.named_type;
    dict[FieldNames::CUSTOM_ATTRIBUTES] = self.custom_attributes;
    return move(dict);
};

/**
 * @brief Converts a Node object to a Python dictionary.
 * @param self The Node object to be converted.
 * @return A Python dictionary (`nb::dict`) containing the attributes of the Node.
 */
static nb::dict node_to_dict(const Node& self) {
    nb::dict dict = atom_to_dict(self);
    dict[FieldNames::NODE_NAME] = self.name;
    return move(dict);
};

/**
 * @brief Converts a Link object to a Python dictionary.
 * @param self The Link object to be converted.
 * @return A Python dictionary (`nb::dict`) containing the attributes of the Link.
 */
static nb::dict link_to_dict(const Link& self) {
    nb::dict dict = atom_to_dict(self);
    dict[FieldNames::COMPOSITE_TYPE] = composite_type_to_pylist(self.composite_type);
    dict[FieldNames::TYPE_NAME_HASH] = self.named_type_hash;
    dict[FieldNames::TARGETS] = self.targets;
    dict[FieldNames::IS_TOPLEVEL] = self.is_toplevel;
    nb::list targets_documents;
    for (const auto& target : self.targets_documents) {
        if (auto node = std::get_if<Node>(&target)) {
            targets_documents.append(node_to_dict(*node));
        } else if (auto link = std::get_if<Link>(&target)) {
            targets_documents.append(link_to_dict(*link));
        }
    }
    dict[FieldNames::TARGETS_DOCUMENTS] = move(targets_documents);
    return move(dict);
};

/**
 * @brief Converts a Node object to a NodeTuple.
 * @param node The Node object to be converted.
 * @return A NodeTuple containing the attributes of the Node.
 */
static NodeTuple node_to_tuple(const Node& node) {
    return std::make_tuple(node._id,
                           node.handle,
                           node.composite_type_hash,
                           node.named_type,
                           node.name,
                           node.custom_attributes);
}

/**
 * @brief Converts a NodeTuple to a Node object.
 * @param node The Node object to be updated.
 * @param state The NodeTuple containing the new state.
 */
static void tuple_to_node(Node& node, const NodeTuple& state) {
    new (&node) Node(std::get<0>(state),  // id
                     std::get<1>(state),  // handle
                     std::get<2>(state),  // composite_type_hash
                     std::get<3>(state),  // named_type
                     std::get<4>(state),  // name
                     std::get<5>(state)   // custom_attributes
    );
}

/**
 * @brief Converts a Link object to a LinkTuple.
 * @param link The Link object to be converted.
 * @return A LinkTuple containing the attributes of the Link.
 */
static LinkTuple link_to_tuple(const Link& link) {
    return std::make_tuple(link._id,
                           link.handle,
                           link.composite_type_hash,
                           link.named_type,
                           composite_type_to_pylist(link.composite_type),
                           link.named_type_hash,
                           link.targets,
                           link.is_toplevel,
                           link.custom_attributes,
                           link.targets_documents);
}

/**
 * @brief Converts a LinkTuple to a Link object.
 * @param link The Link object to be updated.
 * @param state The LinkTuple containing the new state.
 */
static void tuple_to_link(Link& link, const LinkTuple& state) {
    new (&link) Link(std::get<0>(state),  // id
                     std::get<1>(state),  // handle
                     std::get<2>(state),  // composite_type_hash
                     std::get<3>(state),  // named_type
                     pylist_to_composite_type(std::get<4>(state)),
                     std::get<5>(state),  // named_type_hash
                     std::get<6>(state),  // targets
                     std::get<7>(state),  // is_toplevel
                     std::get<8>(state),  // custom_attributes
                     std::get<9>(state)   // targets_documents
    );
}

/**
 * @brief Initializes a Link object with the provided parameters.
 * @param self The Link object to be initialized.
 * @param _id The ID of the link.
 * @param handle The handle of the link.
 * @param composite_type_hash The composite type hash of the link.
 * @param named_type The named type of the link.
 * @param composite_type The composite type list of the link.
 * @param named_type_hash The named type hash of the link.
 * @param targets The targets of the link.
 * @param is_toplevel Indicates if the link is top-level.
 * @param targets_documents Optional targets documents associated with the link.
 * @param custom_attributes Optional custom attributes associated with the link.
 */
static void init_link(Link& self,
                      const string& _id,
                      const string& handle,
                      const string& composite_type_hash,
                      const string& named_type,
                      const nb::list& composite_type,
                      const string& named_type_hash,
                      const vector<string>& targets,
                      bool is_toplevel,
                      const CustomAttributes& custom_attributes = CustomAttributes{},
                      const Link::TargetsDocuments& targets_documents = Link::TargetsDocuments{}) {
    new (&self) Link(_id,
                     handle,
                     composite_type_hash,
                     named_type,
                     pylist_to_composite_type(composite_type),
                     named_type_hash,
                     targets,
                     is_toplevel,
                     custom_attributes,
                     targets_documents);
}

}  // namespace bind_helpers
