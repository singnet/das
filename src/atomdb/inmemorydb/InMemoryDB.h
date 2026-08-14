#pragma once

#include <map>
#include <memory>
#include <mutex>
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

/**
 * In-memory AtomDB backed by HandleTries.
 *
 * Thread-safety model:
 * - Pure reads (get_*, query_*, *_exist(s), counts, get_all_atoms) take no InMemoryDB-wide
 *   lock. Each read snapshots the trie shared_ptrs (guarded by a leaf pointer mutex) and
 *   relies on HandleTrie's hand-over-hand per-node locking for the traversal itself.
 * - Mutations (add_*, delete_*, re_index_patterns, drop_all, add_pattern_index_schema) are
 *   serialized by write_mutex_ so the three tries stay mutually consistent.
 * - drop_all() and re_index_patterns(true) never delete tries in place: they build fresh
 *   tries and swap the shared_ptrs. Readers holding a pre-swap snapshot keep a live trie
 *   until they finish; it is freed when the last snapshot is dropped.
 *
 * Relaxed guarantee: a concurrent reader may briefly observe an atom before its pattern /
 * incoming-set index entries exist (writes stay atomic w.r.t. each other).
 */
class InMemoryDB : public AtomDB {
   public:
    InMemoryDB(const string& context = "");
    ~InMemoryDB();

    bool allow_nested_indexing() override;
    bool composite_type_enabled() const override { return false; }
    atomdb_api_types::ProtectionMode is_protected() const override {
        return atomdb_api_types::ProtectionMode::UNPROTECTED;
    }

    shared_ptr<Atom> get_atom(const string& handle) override;
    shared_ptr<Node> get_node(const string& handle) override;
    shared_ptr<Link> get_link(const string& handle) override;

    vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel, Atom& key) override;

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema) override;

    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle) override;

    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle) override;

    bool atom_exists(const string& handle) override;
    bool node_exists(const string& handle) override;
    bool link_exists(const string& handle) override;

    set<string> atoms_exist(const vector<string>& handles) override;
    set<string> nodes_exist(const vector<string>& handles) override;
    set<string> links_exist(const vector<string>& handles) override;

    string add_atom(const atoms::Atom* atom, const atoms::Merger* merger = NULL) override;
    string add_node(const atoms::Node* node, const atoms::Merger* merger = NULL) override;
    string add_link(const atoms::Link* link, const atoms::Merger* merger = NULL) override;

    vector<string> add_atoms(const vector<atoms::Atom*>& atom_list,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;
    vector<string> add_links(const vector<atoms::Link*>& links,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override;

    bool delete_atom(const string& handle, bool delete_link_targets = false) override;
    bool delete_node(const string& handle, bool delete_link_targets = false) override;
    bool delete_link(const string& handle, bool delete_link_targets = false) override;

    uint delete_atoms(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_nodes(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_links(const vector<string>& handles, bool delete_link_targets = false) override;

    size_t node_count() const override;
    size_t link_count() const override;
    size_t atom_count() const override;

    void re_index_patterns(bool flush_patterns = true) override;

    /** Returns deep clones of every stored atom. */
    vector<shared_ptr<Atom>> get_all_atoms();
    /** Removes all stored atoms and resets all indexes. */
    void drop_all();

    void add_pattern(const string& pattern_handle, const string& atom_handle);
    vector<string> match_pattern_index_schema(const Link* link);

   private:
    // Trie snapshot accessors. Safe to use outside any lock: swaps happen only under
    // trie_ptr_mutex_ (and write_mutex_), and old snapshots stay alive via shared_ptr.
    shared_ptr<HandleTrie> atoms_trie() const;
    shared_ptr<HandleTrie> pattern_index_trie() const;
    shared_ptr<HandleTrie> incoming_sets_trie() const;

    // Unlocked helpers — caller must hold write_mutex_.
    string add_node_unlocked(const atoms::Node* node, const atoms::Merger* merger);
    vector<string> add_nodes_unlocked(const vector<atoms::Node*>& nodes,
                                      bool is_transactional,
                                      const atoms::Merger* merger);
    vector<string> add_links_unlocked(const vector<atoms::Link*>& links,
                                      bool is_transactional,
                                      const atoms::Merger* merger);

    bool delete_atom_unlocked(const string& handle, bool delete_link_targets);
    bool delete_node_unlocked(const string& handle, bool delete_link_targets);
    bool delete_link_unlocked(const string& handle, bool delete_link_targets);

    static void add_pattern_to(HandleTrie& trie,
                               const string& pattern_handle,
                               const string& atom_handle);
    void delete_pattern_unlocked(const string& pattern_handle, const string& atom_handle);
    void add_incoming_set_unlocked(const string& target_handle, const string& link_handle);
    void delete_incoming_set_unlocked(const string& target_handle, const string& link_handle);
    vector<string> match_pattern_index_schema_unlocked(const Link* link);
    vector<vector<string>> index_entries_combinations(unsigned int arity);
    void add_pattern_index_schema(const string& tokens, const vector<vector<string>>& index_entries);

    string context_;
    // Serializes mutations across the three tries. Reads never take it.
    mutable mutex write_mutex_;
    // Leaf mutex guarding only the trie shared_ptr swaps below. Never held across trie ops.
    mutable mutex trie_ptr_mutex_;
    shared_ptr<HandleTrie> atoms_trie_;          // Stores handle -> Atom*
    shared_ptr<HandleTrie> pattern_index_trie_;  // Stores pattern_handle -> set of atom handles
    shared_ptr<HandleTrie> incoming_sets_trie_;  // Stores target_handle -> set of link handles

    map<int, tuple<vector<string>, vector<vector<string>>>> pattern_index_schema_map;
    int pattern_index_schema_next_priority{1};
};

}  // namespace atomdb
