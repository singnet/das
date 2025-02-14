#pragma once

#include <database.h>
#include <nanobind/trampoline.h>

using namespace std;
using namespace atomdb;

namespace nb = nanobind;

/**
 * @brief A trampoline struct for the AtomDB interface.
 *
 * This struct provides concrete implementations for the pure virtual methods
 * defined in the AtomDB interface. It is used to facilitate the integration
 * of the AtomDB interface with other components, such as Python bindings.
 * The AtomDBTrampoline struct inherits from AtomDB and overrides its methods,
 * allowing for custom behavior and additional functionality.
 */
struct AtomDBTrampoline : AtomDB {
    NB_TRAMPOLINE(AtomDB, 32);

    const string get_node_handle(const string& node_type, const string& node_name) const override {
        NB_OVERRIDE_PURE(get_node_handle, node_type, node_name);
    }

    const string get_node_name(const string& node_handle) const override {
        NB_OVERRIDE_PURE(get_node_name, node_handle);
    }

    const string get_node_type(const string& node_handle) const override {
        NB_OVERRIDE_PURE(get_node_type, node_handle);
    }

    const StringList get_node_by_name(const string& node_type, const string& substring) const override {
        NB_OVERRIDE_PURE(get_node_by_name, node_type, substring);
    }

    const StringList get_atoms_by_field(
        const vector<unordered_map<string, string>>& query) const override {
        NB_OVERRIDE_PURE(get_atoms_by_field, query);
    }

    const pair<const int, const AtomList> get_atoms_by_index(const string& index_id,
                                                             const vector<map<string, string>>& query,
                                                             int cursor = 0,
                                                             int chunk_size = 500) const override {
        NB_OVERRIDE_PURE(get_atoms_by_index, index_id, query, cursor, chunk_size);
    }

    const StringList get_atoms_by_text_field(
        const string& text_value,
        const optional<string>& field = nullopt,
        const optional<string>& text_index_id = nullopt) const override {
        NB_OVERRIDE_PURE(get_atoms_by_text_field, text_value, field, text_index_id);
    }

    const StringList get_node_by_name_starting_with(const string& node_type,
                                                    const string& startswith) const override {
        NB_OVERRIDE_PURE(get_node_by_name_starting_with, node_type, startswith);
    }

    const StringList get_all_nodes_handles(const string& node_type) const override {
        NB_OVERRIDE_PURE(get_all_nodes_handles, node_type);
    }

    const StringList get_all_nodes_names(const string& node_type) const override {
        NB_OVERRIDE_PURE(get_all_nodes_names, node_type);
    }

    const StringUnorderedSet get_all_links(const string& link_type) const override {
        NB_OVERRIDE_PURE(get_all_links, link_type);
    }

    const string get_link_handle(const string& link_type,
                                 const StringList& target_handles) const override {
        NB_OVERRIDE_PURE(get_link_handle, link_type, target_handles);
    }

    const string get_link_type(const string& link_handle) const override {
        NB_OVERRIDE_PURE(get_link_type, link_handle);
    }

    const StringList get_link_targets(const string& link_handle) const override {
        NB_OVERRIDE_PURE(get_link_targets, link_handle);
    }

    const StringList get_incoming_links_handles(const string& atom_handle,
                                                const KwArgs& kwargs = {}) const override {
        NB_OVERRIDE_PURE(get_incoming_links_handles, atom_handle, kwargs);
    }

    const vector<shared_ptr<const Atom>> get_incoming_links_atoms(
        const string& atom_handle, const KwArgs& kwargs = {}) const override {
        NB_OVERRIDE_PURE(get_incoming_links_atoms, atom_handle, kwargs);
    }

    const StringUnorderedSet get_matched_links(const string& link_type,
                                               const StringList& target_handles,
                                               const KwArgs& kwargs = {}) const override {
        NB_OVERRIDE_PURE(get_matched_links, link_type, target_handles, kwargs);
    }

    const StringUnorderedSet get_matched_type_template(const StringList& _template,
                                                       const KwArgs& kwargs = {}) const override {
        NB_OVERRIDE_PURE(get_matched_type_template, _template, kwargs);
    }

    const StringUnorderedSet get_matched_type(const string& link_type,
                                              const KwArgs& kwargs = {}) const override {
        NB_OVERRIDE_PURE(get_matched_type, link_type, kwargs);
    }

    const optional<const string> get_atom_type(const string& handle) const override {
        NB_OVERRIDE_PURE(get_atom_type, handle);
    }

    const unordered_map<string, int> count_atoms() const override { NB_OVERRIDE_PURE(count_atoms); }

    void clear_database() override { NB_OVERRIDE_PURE(clear_database); }

    const shared_ptr<const Node> add_node(const Node& node_params) override {
        NB_OVERRIDE_PURE(add_node, node_params);
    }

    const shared_ptr<const Link> add_link(const Link& link_params, bool toplevel = true) override {
        NB_OVERRIDE_PURE(add_link, link_params, toplevel);
    }

    void reindex(const unordered_map<string, vector<unordered_map<string, any>>>&
                     pattern_index_templates) override {
        NB_OVERRIDE_PURE(reindex, pattern_index_templates);
    }

    void delete_atom(const string& handle) override { NB_OVERRIDE_PURE(delete_atom, handle); }

    const string create_field_index(const string& atom_type,
                                    const StringList& fields,
                                    const string& named_type = "",
                                    const optional<const StringList>& composite_type = nullopt,
                                    FieldIndexType index_type = FieldIndexType::BINARY_TREE) override {
        NB_OVERRIDE_PURE(create_field_index, atom_type, fields, named_type, composite_type, index_type);
    }

    void bulk_insert(const vector<shared_ptr<const Atom>>& documents) override {
        NB_OVERRIDE_PURE(bulk_insert, documents);
    }

    const vector<shared_ptr<const Atom>> retrieve_all_atoms() const override {
        NB_OVERRIDE_PURE(retrieve_all_atoms);
    }

    void commit(const optional<const vector<Atom>>& buffer = nullopt) override {
        NB_OVERRIDE_PURE(commit, buffer);
    }

    const shared_ptr<const Atom> _get_atom(const string& handle) const override {
        NB_OVERRIDE_PURE(_get_atom, handle);
    }

    shared_ptr<Link> _build_link(const Link& link_params, bool is_toplevel = true) override {
        NB_OVERRIDE(_build_link, link_params, is_toplevel);
    }
};
