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

    bool allow_nested_indexing(const string& public_key) override;
    bool composite_type_enabled() const override { return false; }
    bool is_protected() const override;

    shared_ptr<Atom> get_atom(const string& handle, const string& public_key) override;
    shared_ptr<Node> get_node(const string& handle, const string& public_key) override;
    shared_ptr<Link> get_link(const string& handle, const string& public_key) override;

    vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel,
                                                Atom& key,
                                                const string& public_key) override;

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema,
                                                              const string& public_key) override;
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle,
                                                               const string& public_key) override;
    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle,
                                                                   const string& public_key) override;

    bool atom_exists(const string& handle, const string& public_key) override;
    bool node_exists(const string& handle, const string& public_key) override;
    bool link_exists(const string& handle, const string& public_key) override;

    set<string> atoms_exist(const vector<string>& handles, const string& public_key) override;
    set<string> nodes_exist(const vector<string>& handles, const string& public_key) override;
    set<string> links_exist(const vector<string>& handles, const string& public_key) override;

    string add_atom(const atoms::Atom* atom,
                    const string& public_key,
                    bool throw_if_exists = false) override;
    string add_node(const atoms::Node* node,
                    const string& public_key,
                    bool throw_if_exists = false) override;
    string add_link(const atoms::Link* link,
                    const string& public_key,
                    bool throw_if_exists = false) override;

    vector<string> add_atoms(const vector<atoms::Atom*>& atoms,
                             const string& public_key,
                             bool throw_if_exists = false,
                             bool is_transactional = false) override;
    vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                             const string& public_key,
                             bool throw_if_exists = false,
                             bool is_transactional = false) override;
    vector<string> add_links(const vector<atoms::Link*>& links,
                             const string& public_key,
                             bool throw_if_exists = false,
                             bool is_transactional = false) override;

    bool delete_atom(const string& handle,
                     const string& public_key,
                     bool delete_link_targets = false) override;
    bool delete_node(const string& handle,
                     const string& public_key,
                     bool delete_link_targets = false) override;
    bool delete_link(const string& handle,
                     const string& public_key,
                     bool delete_link_targets = false) override;

    uint delete_atoms(const vector<string>& handles,
                      const string& public_key,
                      bool delete_link_targets = false) override;
    uint delete_nodes(const vector<string>& handles,
                      const string& public_key,
                      bool delete_link_targets = false) override;
    uint delete_links(const vector<string>& handles,
                      const string& public_key,
                      bool delete_link_targets = false) override;

    void re_index_patterns(const string& public_key, bool flush_patterns = true) override;

    size_t node_count(const string& public_key) const override;
    size_t link_count(const string& public_key) const override;
    size_t atom_count(const string& public_key) const override;

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
