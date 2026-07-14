#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "AtomDB.h"
#include "DedicatedThread.h"
#include "HandleTrie.h"
#include "InMemoryDB.h"
#include "LinkSchema.h"

using namespace std;
using namespace commons;
using namespace atoms;

namespace atomdb {

/**
 * RemoteAtomDBPeer represents a connection to a remote AtomDB with optional layers.
 * It may combine an in-memory cache, a read-only remote AtomDB backend, and optional
 * local persistence for newly added atoms. If readonly is true, the peer doest not allow any
 * mutations to the remote backend.
 */
class RemoteAtomDBPeer : public AtomDB, public processor::ThreadMethod {
   public:
    RemoteAtomDBPeer(shared_ptr<AtomDB> remote_atomdb,
                     shared_ptr<AtomDB> local_persistence = nullptr,
                     const string& uid = "");
    ~RemoteAtomDBPeer();

    bool allow_nested_indexing() override;

    shared_ptr<Atom> get_atom(const string& handle) override;
    shared_ptr<Node> get_node(const string& handle) override;
    shared_ptr<Link> get_link(const string& handle) override;

    // Cache-only lookups (in-memory, no local_persistence / remote escalation). Used by the
    // RemoteAtomDB facade to probe every peer's cache before escalating any peer to its backend.
    shared_ptr<Atom> get_cached_atom(const string& handle);
    shared_ptr<Node> get_cached_node(const string& handle);
    shared_ptr<Link> get_cached_link(const string& handle);

    vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel, Atom& key) override;
    vector<shared_ptr<Atom>> get_matching_atoms(bool is_toplevel, Atom& key, bool local_only);

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema) override;
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle) override;
    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle) override;

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

    size_t node_count() const override;
    size_t link_count() const override;
    size_t atom_count() const override;

    // Cache policy API
    void fetch(const LinkSchema& link_schema);
    void release(const LinkSchema& link_schema, bool persist = true, bool force = false);
    void release_cache(bool persist_to_local, bool persist_entire_cache);
    double available_ram();
    void auto_cleanup();
    void start_cleanup_thread();
    void stop_cleanup_thread();

    // ThreadMethod interface
    bool thread_one_step() override;

    const string& get_uid() const { return uid_; }
    bool is_readonly() const { return local_persistence_ == nullptr; }

   private:
    void feed_cache_from_handle_set(shared_ptr<atomdb_api_types::HandleSet> handle_set);
    void merge_handle_set(shared_ptr<atomdb_api_types::HandleSet> source,
                          shared_ptr<atomdb_api_types::HandleSetInMemory> dest,
                          set<string>& seen,
                          bool copy_metadata = false);
    bool schema_already_fetched(const LinkSchema& link_schema);
    void invalidate_fetched_templates_locked();
    void persist_atoms_to_local(const vector<shared_ptr<atoms::Atom>>& atoms);

    string uid_;
    shared_ptr<InMemoryDB> cache_;
    shared_ptr<AtomDB> atomdb_;
    shared_ptr<AtomDB> local_persistence_;
    unique_ptr<HandleTrie> fetched_link_templates_;
    set<string> staged_handles_;

    unique_ptr<processor::DedicatedThread> cleanup_thread_;
    // InMemoryDB and HandleTrie lock internal operations, but this peer mutates multiple structures
    // as one logical state transition (cache + fetched templates + staged handles), so we keep a
    // coarse lock to preserve consistency across them.
    mutable recursive_mutex cache_mutex_;

    static constexpr double CRITICAL_RAM_THRESHOLD = 0.1;  // 10% - cleanup when below this
};

}  // namespace atomdb
