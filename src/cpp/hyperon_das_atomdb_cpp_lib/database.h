/**
 * @file database.h
 * @brief Abstract base class for the AtomDB database interface.
 *
 * This header file defines the AtomDB class, which serves as an abstract base class for
 * interacting with the AtomDB database. The AtomDB class provides a comprehensive interface
 * for querying, adding, and managing atoms, nodes, and links within the database. Derived
 * classes must implement the pure virtual methods to provide concrete functionality.
 *
 * The AtomDB interface supports various operations, including:
 * - Retrieving atoms by different criteria (e.g., field, index, text field).
 * - Managing nodes and links, including adding, deleting, and querying them.
 * - Reindexing the database and creating field indexes.
 * - Bulk inserting atoms and committing changes.
 *
 * The class is designed to be extended by specific database implementations, allowing for
 * flexible and customizable database interactions. It includes static methods for building
 * node and link handles, as well as methods for checking the existence of nodes and links,
 * retrieving atoms, and reformatting documents.
 *
 * The AtomDB class also defines several pure virtual methods that must be implemented by
 * derived classes. These methods cover a wide range of database operations, such as
 * retrieving node handles, querying the database by field or text, managing links,
 * reindexing, and more.
 *
 * @note This class uses several custom types and utility classes, such as StringList,
 *       ExpressionHasher, NodeParams, LinkParams, and KwArgs, which are defined elsewhere
 *       in the project.
 */

#pragma once

#include "constants.h"
#include "document_types.h"
#include "exceptions.h"
#include "expression_hasher.h"
#include "type_aliases.h"

using namespace std;

namespace atomdb {

/**
 * @brief A Plain Old Data (POD) type representing various boolean flags for configuration options.
 *
 * This structure contains several boolean flags that control different aspects of the
 * configuration, such as target formatting, document handling, representation depth,
 * and scope of operation.
 */
struct KwArgs {
    bool no_target_format = false;
    bool targets_document = false;
    bool deep_representation = false;
    bool toplevel_only = false;
    bool handles_only = false;
};

/**
 * @brief Abstract base class for the AtomDB database interface.
 *
 * The AtomDB class defines the interface for interacting with the AtomDB database.
 * It provides various pure virtual methods for querying, adding, and managing atoms,
 * nodes, and links within the database. Derived classes must implement these methods
 * to provide concrete functionality.
 *
 * The AtomDB interface supports operations such as:
 * - Retrieving atoms by various criteria (e.g., field, index, text field).
 * - Managing nodes and links, including adding, deleting, and querying them.
 * - Reindexing the database and creating field indexes.
 * - Bulk inserting atoms and committing changes.
 *
 * This class is designed to be extended by specific database implementations,
 * allowing for flexible and customizable database interactions.
 */
class AtomDB {
   public:
    AtomDB() = default;
    virtual ~AtomDB() = default;

    /**
     * @brief Build a node handle with the specified type and name.
     * @param node_type The node type.
     * @param node_name The node name.
     * @return The node handle.
     */
    static const string build_node_handle(const string& node_type, const string& node_name) {
        return ExpressionHasher::terminal_hash(node_type, node_name);
    }

    /**
     * @brief Build a link handle with the specified type and single target.
     * @param link_type The link type.
     * @param target_handle A single target handle.
     * @return The link handle.
     */
    static const string build_link_handle(const string& link_type, const string& target_handle) {
        string link_type_hash = ExpressionHasher::named_type_hash(link_type);
        return ExpressionHasher::composite_hash(
            target_handle.empty() ? StringList{{move(link_type_hash)}}
                                  : StringList{{move(link_type_hash)}, {target_handle}});
    }

    /**
     * @brief Build a link handle with the specified type and multiple targets.
     * @param link_type The link type.
     * @param target_handles A list of link target identifiers.
     * @return The link handle.
     */
    static const string build_link_handle(const string& link_type, const StringList& target_handles) {
        string link_type_hash = ExpressionHasher::named_type_hash(link_type);
        return ExpressionHasher::expression_hash(link_type_hash, target_handles);
    }

    /**
     * @brief Checks if a node with the specified type and name exists.
     * @param node_type A string representing the type of the node.
     * @param node_name A string representing the name of the node.
     * @return A boolean value indicating whether the node exists (true) or not (false).
     */
    bool node_exists(const string& node_type, const string& node_name) const;

    /**
     * @brief Checks if a link with the specified type and target handles exists.
     * @param link_type A string representing the type of the link.
     * @param target_handles A vector of strings representing the handles of the link's targets.
     * @return A boolean value indicating whether the link exists (true) or not (false).
     */
    bool link_exists(const string& link_type, const StringList& target_handles) const;

    /**
     * @brief Retrieves an atom from the database using its handle and optional params.
     * @param handle A string representing the handle of the atom to be retrieved.
     * @param kwargs An optional Kwargs object containing additional retrieval options, as follows:
     *               `no_target_format` (`bool`, optional): If True, return the document without
     *                   transforming it to the target format. Defaults to False.
     *               `targets_document` (`bool`, optional): If True, include the `targets_document`
     *                   in the response. Defaults to False.
     *               `deep_representation` (`bool`, optional): If True, include a deep
     *                   representation of the targets. Defaults to False.
     * @return A const shared pointer to an Atom object.
     */
    const shared_ptr<const Atom> get_atom(const string& handle, const KwArgs& kwargs = {}) const;

    // PURE VIRTUAL PUBLIC METHODS /////////////////////////////////////////////////////////////////

    /**
     * @brief Get the handle of the node with the specified type and name.
     * @param node_type The node type.
     * @param node_name The node name.
     * @return The node handle.
     */
    virtual const string get_node_handle(const string& node_type, const string& node_name) const = 0;

    /**
     * @brief Get the name of the node with the specified handle.
     * @param node_handle The node handle.
     * @return The node name.
     */
    virtual const string get_node_name(const string& node_handle) const = 0;

    /**
     * @brief Get the type of the node with the specified handle.
     * @param node_handle The node handle.
     * @return The node type.
     */
    virtual const string get_node_type(const string& node_handle) const = 0;

    /**
     * @brief Get the handles of (a) node(s) of the specified type containing the given substring.
     * @param node_type The node type.
     * @param substring The substring to search for in node names.
     * @return List of handles of nodes whose names matched the criteria.
     */
    virtual const StringList get_node_by_name(const string& node_type,
                                              const string& substring) const = 0;

    /**
     * @brief Query the database by field and value.
     * @param query List of maps containing 'field' and 'value' keys.
     * @return List of node handles.
     */
    virtual const StringList get_atoms_by_field(
        const vector<unordered_map<string, string>>& query) const = 0;

    /**
     * @brief Retrieves atoms from the database using the specified index.
     * @param index_id The ID of the index to use for retrieving atoms from the database.
     * @param query A vector of ordered maps representing the query parameters.
     * @param cursor An integer representing the cursor position for pagination (default is 0).
     * @param chunk_size An integer representing the number of atoms to retrieve in one chunk
     *                   (default is 500).
     * @return A pair containing an cursor and a list of atoms. The cursor is used for pagination
     *         or further retrieval operations, and the list contains the retrieved atoms.
     */
    virtual const pair<const int, const AtomList> get_atoms_by_index(
        const string& index_id,
        const vector<map<string, string>>& query,
        int cursor = 0,
        int chunk_size = 500) const = 0;

    /**
     * @brief Query the database by a text field.
     * @param text_value Value to search for.
     * @param field Field to be used to search.
     * @param text_index_id Index id to search for.
     * @return List of node handles ordered by the closest match.
     */
    virtual const StringList get_atoms_by_text_field(
        const string& text_value,
        const optional<string>& field = nullopt,
        const optional<string>& text_index_id = nullopt) const = 0;

    /**
     * @brief Query the database by node name starting with 'startswith' value.
     * @param node_type The node type.
     * @param startswith The starting substring to search for.
     * @return List of node handles.
     */
    virtual const StringList get_node_by_name_starting_with(const string& node_type,
                                                            const string& startswith) const = 0;

    /**
     * @brief Get all nodes handles of a specific type.
     * @param node_type The node type.
     * @return A list of node handles.
     */
    virtual const StringList get_all_nodes_handles(const string& node_type) const = 0;

    /**
     * @brief Get all nodes names of a specific type.
     * @param node_type The node type.
     * @return A list of names.
     */
    virtual const StringList get_all_nodes_names(const string& node_type) const = 0;

    /**
     * @brief Retrieves all links of the specified type from the database.
     * @param link_type A string representing the type of the links to retrieve.
     * @return A set of handles representing the links.
     */
    virtual const StringUnorderedSet get_all_links(const string& link_type) const = 0;

    /**
     * @brief Get the handle of the link with the specified type and targets.
     * @param link_type The link type.
     * @param target_handles A list of link target identifiers.
     * @return The link handle.
     */
    virtual const string get_link_handle(const string& link_type,
                                         const StringList& target_handles) const = 0;

    /**
     * @brief Get the type of the link with the specified handle.
     * @param link_handle The link handle.
     * @return The link type.
     */
    virtual const string get_link_type(const string& link_handle) const = 0;

    /**
     * @brief Get the target handles of a link specified by its handle.
     * @param link_handle The link handle.
     * @return A list of target handles of the link.
     */
    virtual const StringList get_link_targets(const string& link_handle) const = 0;

    /**
     * @brief Retrieves incoming link handles for the specified atom.
     * @param atom_handle A string representing the handle of the atom.
     * @param kwargs An const reference to a Kwargs object containing additional retrieval options.
     * @return A list of strings representing the incoming link handles.
     */
    virtual const StringList get_incoming_links_handles(const string& atom_handle,
                                                        const KwArgs& kwargs = {}) const = 0;

    /**
     * @brief Retrieves incoming link atoms for the specified atom.
     * @param atom_handle A string representing the handle of the atom.
     * @param kwargs An const reference to a Kwargs object containing additional retrieval options.
     * @return A list of Atom objects representing the incoming links.
     */
    virtual const vector<shared_ptr<const Atom>> get_incoming_links_atoms(
        const string& atom_handle, const KwArgs& kwargs = {}) const = 0;

    /**
     * @brief Retrieves matched links of the specified type and target handles.
     * @param link_type A string representing the type of the links to retrieve.
     * @param target_handles A list of strings representing the target handles.
     * @param kwargs An const reference to a Kwargs object containing additional retrieval options.
     * @return A set of handles that matched.
     */
    virtual const StringUnorderedSet get_matched_links(const string& link_type,
                                                       const StringList& target_handles,
                                                       const KwArgs& kwargs = {}) const = 0;

    /**
     * @brief Retrieves matched type templates based on the specified template.
     * @param _template A list of strings representing the template.
     * @param kwargs An const reference to a Kwargs object containing additional retrieval options.
     * @return A set of handles that matched.
     */
    virtual const StringUnorderedSet get_matched_type_template(const StringList& _template,
                                                               const KwArgs& kwargs = {}) const = 0;

    /**
     * @brief Retrieves matched types based on the specified link type.
     * @param link_type A string representing the type of the links to retrieve.
     * @param kwargs A const reference to a Kwargs object containing additional retrieval options.
     * @return A set of handles that matched.
     */
    virtual const StringUnorderedSet get_matched_type(const string& link_type,
                                                      const KwArgs& kwargs = {}) const = 0;

    /**
     * @brief Retrieves the type of the atom with the specified handle.
     * @param handle A string representing the handle of the atom.
     * @return An optional string containing the type of the atom if found, otherwise nullopt.
     */
    virtual const optional<const string> get_atom_type(const string& handle) const = 0;

    /**
     * @brief Count the total number of atoms in the database.
     * @return A dictionary containing the count of node atoms, link atoms, and total atoms.
     */
    virtual const unordered_map<string, int> count_atoms() const = 0;

    /**
     * @brief Clear the entire database, removing all data.
     */
    virtual void clear_database() = 0;

    /**
     * @brief Adds a node to the database.
     * @param node_params A NodeParams object containing the parameters for the node.
     * @return A const shared pointer to a Node object.
     */
    virtual const shared_ptr<const Node> add_node(const Node& node_params) = 0;

    /**
     * @brief Adds a link to the database.
     * @param link_params A LinkParams object containing the parameters for the link.
     * @param toplevel A boolean indicating whether the link is a top-level link (default is true).
     * @return A const shared pointer to a Link object.
     */
    virtual const shared_ptr<const Link> add_link(const Link& link_params, bool toplevel = true) = 0;

    /**
     * @brief Reindexes the inverted pattern index according to the passed templates.
     *
     * This function reindexes the inverted pattern index based on the specified pattern
     * templates. The pattern templates are specified by atom type in a map, where each
     * atom type maps to a pattern template.
     *
     * @param pattern_index_templates A map where the keys are atom types and the values
     *        are pattern templates. Each pattern template is a vector of maps, where each
     *        map specifies a pattern template with:
     *        - "named_type": A boolean indicating whether the named type should be included.
     *        - "selected_positions": A vector of integers specifying the selected positions.
     *
     * Pattern templates are applied to each link entered in the atom space to determine
     * which entries should be created in the inverted pattern index. Entries in the inverted
     * pattern index are like patterns where the link type and each of its targets may be
     * replaced by wildcards. For instance, given a similarity link Similarity(handle1, handle2),
     * it could be used to create any of the following entries in the inverted pattern index:
     *
     * - *(handle1, handle2)
     *
     * - Similarity(*, handle2)
     *
     * - Similarity(handle1, *)
     *
     * - Similarity(*, *)
     *
     * If we create all possibilities of index entries for all links, the pattern index size
     * will grow exponentially, so we limit the entries we want to create by each type of link.
     * This is what a pattern template for a given link type is. For instance, if we apply this
     * pattern template:
     *
     * @code
     * `{ "named_type": false, "selected_positions": {0, 1} }`
     * @endcode
     *
     * to Similarity(handle1, handle2), we'll create only the following entries:
     *
     * - Similarity(*, handle2)
     *
     * - Similarity(handle1, *)
     *
     * - Similarity(*, *)
     *
     * If we apply this pattern template instead:
     *
     * @code
     * `{ "named_type": true, "selected_positions": {1} }`
     * @endcode
     *
     * We'll have:
     *
     * - *(handle1, handle2)
     *
     * - Similarity(handle1, *)
     */
    virtual void reindex(
        const unordered_map<string, vector<unordered_map<string, any>>>& pattern_index_templates) = 0;

    /**
     * @brief Delete an atom from the database.
     * @param handle Atom handle.
     */
    virtual void delete_atom(const string& handle) = 0;

    /**
     * @brief Create an index for the specified fields in the database.
     * @param atom_type The type of the atom for which the index is created.
     * @param fields A list of fields to be indexed.
     * @param named_type The named type of the atom.
     * @param composite_type A list representing the composite type of the atom.
     * @param index_type The type of the index to create.
     * @return The ID of the created index.
     */
    virtual const string create_field_index(const string& atom_type,
                                            const StringList& fields,
                                            const string& named_type = "",
                                            const optional<const StringList>& composite_type = nullopt,
                                            FieldIndexType index_type = FieldIndexType::BINARY_TREE) = 0;

    /**
     * @brief Insert multiple documents into the database.
     * @param documents A list of Atom objects, each representing a document to be inserted into the db.
     */
    virtual void bulk_insert(const vector<shared_ptr<const Atom>>& documents) = 0;

    /**
     * @brief Retrieve all atoms from the database.
     * @return A list of dictionaries representing the atoms.
     */
    virtual const vector<shared_ptr<const Atom>> retrieve_all_atoms() const = 0;

    /**
     * @brief Commit the current state of the database.
     */
    virtual void commit(const optional<const vector<Atom>>& buffer = nullopt) = 0;

    /**
     * @brief Reformats a document based on the provided params.
     * @param document A shared pointer to the Atom object to be reformatted.
     * @param kwargs A const reference to a Kwargs object containing the reformatting options.
     * @return A shared pointer with a copy of the reference object but with the new format.
     */
    const shared_ptr<const Atom> _reformat_document(const shared_ptr<const Atom>& document,
                                                    const KwArgs& kwargs = {}) const;

   protected:
    // PROTECTED METHODS ///////////////////////////////////////////////////////////////////////////

    /**
     * @brief Builds a node with the specified parameters.
     * @param node_params A NodeParams object containing the parameters for the node.
     * @return A shared pointer to a Node object.
     */
    shared_ptr<Node> _build_node(const Node& node_params);

    /**
     * @brief Builds a link with the specified parameters.
     * @param link_params A LinkParams object containing the parameters for the link.
     * @param is_toplevel A boolean indicating whether the link is a top-level link.
     * @return A shared pointer to a Link object.
     */
    virtual shared_ptr<Link> _build_link(const Link& link_params, bool is_toplevel = true);

    /**
     * @brief Retrieves an atom from the database using its handle.
     * @param handle A string representing the handle of the atom to be retrieved.
     * @return A const shared pointer to the Atom object representing the retrieved atom.
     */
    virtual const shared_ptr<const Atom> _get_atom(const string& handle) const = 0;
};

}  // namespace atomdb
