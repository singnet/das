/**
 * @file ram_only.h
 * @brief Defines in-memory database classes for managing atoms, nodes, and links.
 *
 * This header file contains the definitions for the `Database` and `InMemoryDB` classes,
 * which provide in-memory storage and management of various data types, including atoms,
 * nodes, and links. These classes are designed to facilitate efficient in-memory operations
 * without the overhead of persistent storage, making them suitable for applications that
 * require fast data access and manipulation.
 *
 * The `Database` class serves as a container for different types of data, using unordered
 * maps to store atom types, nodes, links, and sets for managing relationships between them.
 * It provides basic functionalities such as initialization and cleanup of the stored data.
 *
 * The `InMemoryDB` class extends the `AtomDB` class and offers a comprehensive set of
 * methods for adding, retrieving, and managing atoms, nodes, links, and their relationships.
 * It includes functionalities for querying data, managing indexes, and handling complex
 * data structures in memory. This class is intended for use in scenarios where data does
 * not need to be persisted across sessions or can be reconstructed from other sources if
 * needed.
 *
 * Key functionalities provided by the `InMemoryDB` class include:
 * - Adding and retrieving nodes and links.
 * - Querying atoms by various criteria.
 * - Managing incoming and outgoing links.
 * - Handling patterns and templates for complex data structures.
 * - Indexing and reindexing data for efficient lookups.
 * - Bulk insertion and retrieval of atoms.
 * - Committing changes to the database.
 *
 * The classes in this module are designed to be flexible and efficient, providing a robust
 * solution for managing in-memory data structures in applications that require high
 * performance and low latency.
 */
#pragma once

#include "database.h"
#include "document_types.h"
#include "type_aliases.h"

using namespace std;

namespace atomdb {

/**
 * @brief Represents an in-memory database for storing and managing atoms, nodes, and links.
 */
class Database {
   public:
    unordered_map<string, shared_ptr<Node>> node;
    unordered_map<string, shared_ptr<Link>> link;
    unordered_map<string, StringList> outgoing_set;
    unordered_map<string, StringUnorderedSet> incoming_set;
    unordered_map<string, StringUnorderedSet> patterns;
    unordered_map<string, StringUnorderedSet> templates;

    Database() : node({}), link({}), outgoing_set({}), incoming_set({}), patterns({}), templates({}) {}

    ~Database() {
        node.clear();
        link.clear();
        outgoing_set.clear();
        incoming_set.clear();
        patterns.clear();
        templates.clear();
    };
};

/**
 * @brief Represents an in-memory database for storing and managing atoms, nodes, and links.
 *
 * The InMemoryDB class inherits from the AtomDB class and provides an implementation for
 * managing various types of data within an in-memory data structure. It supports operations
 * such as adding, retrieving, and clearing atoms, nodes, links, and their relationships.
 *
 * This class uses unordered maps to store different types of data, including atom types,
 * nodes, links, and sets for managing relationships between atoms. It is designed to be
 * efficient for in-memory operations, making it suitable for applications that require
 * fast access to data without the overhead of persistent storage.
 *
 * The InMemoryDB class is intended to be used in scenarios where data does not need to be
 * persisted across sessions, or where the data can be reconstructed from other sources if
 * needed. It provides a flexible and efficient way to manage complex data structures in
 * memory.
 */
class InMemoryDB : public AtomDB {
   public:
    InMemoryDB(const string& database_name = "das") : database_name(database_name){};
    ~InMemoryDB(){};

    const string get_node_handle(const string& node_type, const string& node_name) const override;

    const string get_node_name(const string& node_handle) const override;

    const string get_node_type(const string& node_handle) const override;

    const StringList get_node_by_name(const string& node_type, const string& substring) const override;

    const StringList get_atoms_by_field(
        const vector<unordered_map<string, string>>& query) const override;

    const pair<const int, const AtomList> get_atoms_by_index(const string& index_id,
                                                             const vector<map<string, string>>& query,
                                                             int cursor = 0,
                                                             int chunk_size = 500) const override;

    const StringList get_atoms_by_text_field(
        const string& text_value,
        const optional<string>& field = nullopt,
        const optional<string>& text_index_id = nullopt) const override;

    const StringList get_node_by_name_starting_with(const string& node_type,
                                                    const string& startswith) const override;

    const StringList get_all_nodes_handles(const string& node_type) const override;

    const StringList get_all_nodes_names(const string& node_type) const override;

    const StringUnorderedSet get_all_links(const string& link_type) const override;

    const string get_link_handle(const string& link_type,
                                 const StringList& target_handles) const override;

    const string get_link_type(const string& link_handle) const override;

    const StringList get_link_targets(const string& link_handle) const override;

    const StringList get_incoming_links_handles(const string& atom_handle,
                                                const KwArgs& kwargs = {}) const override;

    const vector<shared_ptr<const Atom>> get_incoming_links_atoms(
        const string& atom_handle, const KwArgs& kwargs = {}) const override;

    const StringUnorderedSet get_matched_links(const string& link_type,
                                               const StringList& target_handles,
                                               const KwArgs& kwargs = {}) const override;

    const StringUnorderedSet get_matched_type_template(const StringList& _template,
                                                       const KwArgs& kwargs = {}) const override;

    const StringUnorderedSet get_matched_type(const string& link_type,
                                              const KwArgs& kwargs = {}) const override;

    const optional<const string> get_atom_type(const string& handle) const override;

    const unordered_map<string, int> count_atoms() const override;

    void clear_database() override;

    const shared_ptr<const Node> add_node(const Node& node_params) override;

    const shared_ptr<const Link> add_link(const Link& link_params, bool toplevel = true) override;

    void reindex(const unordered_map<string, vector<unordered_map<string, any>>>&
                     pattern_index_templates) override;

    void delete_atom(const string& handle) override;

    const string create_field_index(const string& atom_type,
                                    const StringList& fields,
                                    const string& named_type = "",
                                    const optional<const StringList>& composite_type = nullopt,
                                    FieldIndexType index_type = FieldIndexType::BINARY_TREE) override;

    void bulk_insert(const vector<shared_ptr<const Atom>>& documents) override;

    const vector<shared_ptr<const Atom>> retrieve_all_atoms() const override;

    void commit(const optional<const vector<Atom>>& buffer = nullopt) override;

   protected:
    string database_name;
    Database db;

    const shared_ptr<const Atom> _get_atom(const string& handle) const override;

    const shared_ptr<const Node> _get_node(const string& handle) const;

    const shared_ptr<const Link> _get_link(const string& handle) const;

    const shared_ptr<const Link> _get_and_delete_link(const string& link_handle);

    /**
     * @brief Builds a named type hash template from the given template.
     * @param _template A list of strings representing the template.
     * @return A list of strings representing the named type hash template.
     */
    const StringList _build_named_type_hash_template(const StringList& _template) const;

    /**
     * @brief Builds a hash for a named type template.
     *
     * This method takes a string representing a named type template and generates
     * a hash for it. The hash can be used to uniquely identify the template within
     * the database, ensuring efficient lookups and comparisons.
     *
     * @param _template The string representation of the named type template.
     * @return A string containing the hash of the named type template.
     */
    const string _build_named_type_hash_template(const string& _template) const;

    /**
     * @brief Adds a set of outgoing links to the database.
     * @param key The key associated with the outgoing links.
     * @param targets_hash The list of target hashes for the outgoing links.
     */
    void _add_outgoing_set(const string& key, const StringList& targets_hash);

    /**
     * @brief Retrieves and deletes the outgoing set associated with a handle.
     * @param handle The handle for which the outgoing set is to be retrieved and deleted.
     * @return An optional StringList containing the outgoing set if it exists.
     */
    const optional<const StringList> _get_and_delete_outgoing_set(const string& handle);

    /**
     * @brief Adds a set of incoming links to the database.
     * @param key The key associated with the incoming links.
     * @param targets_hash The list of target hashes for the incoming links.
     */
    void _add_incoming_set(const string& key, const StringList& targets_hash);

    /**
     * @brief Deletes a set of incoming atoms associated with a given link handle.
     * @param link_handle A string representing the handle of the link.
     * @param atoms_handles A list of strings representing the handles of the atoms to be deleted.
     */
    void _delete_incoming_set(const string& link_handle, const StringList& atoms_handles);

    /**
     * @brief Adds templates to the internal storage.
     * @param composite_type_hash The hash of the composite type.
     * @param named_type_hash The hash of the named type.
     * @param key The key associated with the template.
     */
    void _add_templates(const string& composite_type_hash,
                        const string& named_type_hash,
                        const string& key);

    /**
     * @brief Deletes templates associated with the given document link.
     * @param link_document The link to the document whose templates are to be deleted.
     */
    void _delete_templates(const Link& link_document);

    /**
     * @brief Adds patterns to the internal storage.
     * @param named_type_hash A string representing the hash of the named type.
     * @param key A string representing the key associated with the patterns.
     * @param targets_hash A list of strings representing the target hashes.
     */
    void _add_patterns(const string& named_type_hash, const string& key, const StringList& targets_hash);

    /**
     * @brief Deletes patterns from the specified document.
     * @param link_document The link to the document from which patterns will be deleted.
     * @param targets_hash A list of hashes representing the patterns to be deleted.
     */
    void _delete_patterns(const Link& link_document, const StringList& targets_hash);

    /**
     * @brief Deletes a link and updates the index accordingly.
     * @param link_handle The handle of the link to be deleted.
     */
    void _delete_link_and_update_index(const string& link_handle);

    /**
     * @brief Filters out non-top-level elements from the given set of matches.
     * @param matches The set of matches to be filtered.
     * @return A set containing only the top-level elements from the input matches.
     */
    const StringUnorderedSet _filter_non_toplevel(const StringUnorderedSet& matches) const;

    /**
     * @brief Deletes the index of the specified atom.
     * @param atom The atom whose index is to be deleted.
     */
    void _delete_atom_index(const Atom& atom);

    /**
     * @brief Adds an atom to the index.
     * @param atom The atom to be added.
     */
    void _add_atom_index(const Atom& atom);

    /**
     * @brief Updates the index for the given atom.
     * @param atom The atom to update the index for.
     * @param delete_atom Flag indicating whether to delete the atom from the index.
     */
    void _update_index(const Atom& atom, bool delete_atom = false);
};

}  // namespace atomdb
