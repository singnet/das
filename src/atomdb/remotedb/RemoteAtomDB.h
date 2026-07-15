#pragma once

#include <map>
#include <memory>
#include <string>

#include "AtomDB.h"
#include "JsonConfig.h"
#include "RemoteAtomDBPeer.h"

using namespace std;

namespace atomdb {

/**
 * RemoteAtomDB connects to multiple remote AtomDBs via RemoteAtomDBPeer instances.
 * Each peer maintains its own optional cache, remote connection, and optional local persistence.
 * The constructor expects a JSON config with connection info for each remote peer.
 */
class RemoteAtomDB : public AtomDB {
   public:
    explicit RemoteAtomDB(const JsonConfig& peers_config);
    /**
     * Dependency-injection constructor for pre-built peers.
     * Primarily used by tests to federate controllable backends without live config/connection.
     */
    explicit RemoteAtomDB(map<string, shared_ptr<RemoteAtomDBPeer>> peers);
    ~RemoteAtomDB();

    bool allow_nested_indexing() override;

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

    const map<string, shared_ptr<RemoteAtomDBPeer>>& get_remote_dbs() const { return remote_db_; }
    RemoteAtomDBPeer* get_peer(const string& uid);

    void release_caches(const LinkSchema& link_schema, bool persist = true, bool force = false);

   private:
    // Derives the aggregated nested-indexing capability from the current peers. Shared by both
    // constructors so the config and DI paths stay consistent.
    void derive_nested_indexing();

    // Wire a federation atom resolver into each writable peer so persist-time target closure
    // can pull missing atoms from other peers (lazy replication of the working set).
    void wire_atom_resolvers();
    shared_ptr<Atom> resolve_atom_from_other_peers(const string& handle, const RemoteAtomDBPeer* self);

    map<string, shared_ptr<RemoteAtomDBPeer>> remote_db_;
    // Aggregated nested-indexing capability, derived from peers at construction. True only when
    // every peer supports nested indexing; mixed configurations are normalized to false.
    bool nested_indexing_ = false;
};

}  // namespace atomdb
