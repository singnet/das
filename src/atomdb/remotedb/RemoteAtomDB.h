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
 * Each peer maintains its own cache, remote connection, and local persistence.
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

    bool allow_nested_indexing(const string& public_key) override;
    bool composite_type_enabled() const override;
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

    const map<string, shared_ptr<RemoteAtomDBPeer>>& get_remote_dbs() const { return remote_db_; }
    RemoteAtomDBPeer* get_peer(const string& uid);

   private:
    // Derives the aggregated nested-indexing capability from the current peers. Shared by both
    // constructors so the config and DI paths stay consistent.
    void derive_nested_indexing();

    map<string, shared_ptr<RemoteAtomDBPeer>> remote_db_;
    // Aggregated nested-indexing capability, derived from peers at construction. True only when
    // every peer supports nested indexing; mixed configurations are normalized to false.
    bool nested_indexing_ = false;
};

}  // namespace atomdb
