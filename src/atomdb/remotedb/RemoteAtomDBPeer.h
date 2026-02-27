#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "AtomDB.h"
#include "HandleTrie.h"
#include "InMemoryDB.h"
#include "LinkSchema.h"

using namespace std;
using namespace commons;
using namespace atoms;

namespace atomdb {

/**
 * RemoteAtomDBPeer represents a cached connection to a remote AtomDB.
 * It combines an in-memory cache, a read-only remote AtomDB, and local persistence
 * for newly added atoms.
 */
class RemoteAtomDBPeer : public AtomDB {
   public:
    RemoteAtomDBPeer(shared_ptr<AtomDB> remote_atomdb,
                     shared_ptr<AtomDB> local_persistence,
                     const string& uid = "");
    ~RemoteAtomDBPeer();

    bool allow_nested_indexing() override;

    shared_ptr<Atom> get_atom(const string& handle) override;
    shared_ptr<Node> get_node(const string& handle) override;
    shared_ptr<Link> get_link(const string& handle) override;

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

    // Cache policy API
    void fetch(const LinkSchema& link_schema);
    void release(const LinkSchema& link_schema);
    double available_ram();
    void auto_cleanup();
    void start_cleanup_thread();
    void stop_cleanup_thread();

    const string& get_uid() const { return uid_; }

   private:
    vector<shared_ptr<Atom>> get_matching_atoms_impl(bool is_toplevel, Atom& key, bool local_only);
    void feed_cache_from_handle_set(shared_ptr<atomdb_api_types::HandleSet> handle_set);
    bool schema_already_fetched(const LinkSchema& link_schema);

    string uid_;
    InMemoryDB cache_;
    shared_ptr<AtomDB> atomdb_;
    shared_ptr<AtomDB> local_persistence_;
    HandleTrie fetched_link_templates_;

    atomic<bool> cleanup_thread_running_{false};
    thread cleanup_thread_;
    mutex cleanup_mutex_;

    static constexpr double CRITICAL_RAM_THRESHOLD = 0.1;  // 10% - cleanup when below this
};

}  // namespace atomdb
