#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "AtomDB.h"
#include "HandleTrie.h"
#include "InMemoryDBAPITypes.h"
#include "LinkSchema.h"

using namespace std;
using namespace commons;
using namespace atoms;

namespace atomdb {

class InMemoryDB : public AtomDB {
   public:
    InMemoryDB(const string& context = "");
    ~InMemoryDB();

    bool allow_nested_indexing() override;

    shared_ptr<Atom> get_atom(const string& handle) override;

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema) override;

    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle) override;

    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle) override;

    shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const string& handle) override;
    shared_ptr<atomdb_api_types::AtomDocument> get_node_document(const string& handle) override;
    shared_ptr<atomdb_api_types::AtomDocument> get_link_document(const string& handle) override;

    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_atom_documents(
        const vector<string>& handles, const vector<string>& fields) override;
    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_node_documents(
        const vector<string>& handles, const vector<string>& fields) override;
    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_link_documents(
        const vector<string>& handles, const vector<string>& fields) override;

    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_matching_atoms(bool is_toplevel,
                                                                          Atom& key) override;

    bool atom_exists(const string& handle) override;
    bool node_exists(const string& handle) override;
    bool link_exists(const string& handle) override;

    set<string> atoms_exist(const vector<string>& handles) override;
    set<string> nodes_exist(const vector<string>& handles) override;
    set<string> links_exist(const vector<string>& handles) override;

    string add_atom(const atoms::Atom* atom, bool throw_if_exists = false) override;
    string add_node(const atoms::Node* node, bool throw_if_exists = false) override;
    string add_link(const atoms::Link* link, bool throw_if_exists = false) override;

    vector<string> add_atoms(const vector<atoms::Atom*>& atoms,
                             bool throw_if_exists = false,
                             bool is_transactional = false) override;
    vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                             bool throw_if_exists = false,
                             bool is_transactional = false) override;
    vector<string> add_links(const vector<atoms::Link*>& links,
                             bool throw_if_exists = false,
                             bool is_transactional = false) override;

    bool delete_atom(const string& handle, bool delete_link_targets = false) override;
    bool delete_node(const string& handle, bool delete_link_targets = false) override;
    bool delete_link(const string& handle, bool delete_link_targets = false) override;

    uint delete_atoms(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_nodes(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_links(const vector<string>& handles, bool delete_link_targets = false) override;

    void re_index_patterns(bool flush_patterns = true) override;

   private:
    string context_;
    HandleTrie* atoms_trie_;          // Stores handle -> Atom*
    HandleTrie* pattern_index_trie_;  // Stores pattern_handle -> set of atom handles
    HandleTrie* incoming_sets_trie_;  // Stores target_handle -> set of link handles that reference it

    map<int, tuple<vector<string>, vector<vector<string>>>> pattern_index_schema_map;
    int pattern_index_schema_next_priority{1};

    // Helper methods
   public:
    void add_pattern(const string& pattern_handle, const string& atom_handle);
    vector<string> match_pattern_index_schema(const Link* link);

   private:
    void delete_pattern(const string& pattern_handle, const string& atom_handle);
    void add_incoming_set(const string& target_handle, const string& link_handle);
    void delete_incoming_set(const string& target_handle, const string& link_handle);
    void update_incoming_set(const string& target_handle, const string& link_handle);

    void add_pattern_index_schema(const string& tokens, const vector<vector<string>>& index_entries);
    vector<vector<string>> index_entries_combinations(unsigned int arity);
};

}  // namespace atomdb
