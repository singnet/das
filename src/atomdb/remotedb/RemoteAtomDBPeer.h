#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include "AtomDB.h"
#include "DedicatedThread.h"
#include "InMemoryDB.h"
#include "LinkSchema.h"

using namespace std;
using namespace commons;
using namespace atoms;

namespace atomdb {

/**
 * RemoteAtomDBPeer represents a connection to a remote AtomDB with optional layers.
 * It may combine:
 * - write_buffer_: locally-added atoms awaiting flush to local_persistence (dirty).
 * - read_cache_: disposable read-through cache of local_persistence / remote hits.
 * - atomdb_: the remote backend
 * - local_persistence_: durable store for mutations (absence => readonly)
 *
 * get_atom order: write_buffer -> local_persistence -> read_cache -> remote.
 * Membership in write_buffer *is* the dirty flag; there is no separate staged set.
 * release_cache() always flushes write_buffer_ (when local_persistence exists) and
 * always drops the read_cache_.
 *
 * Thread-safety: InMemoryDB is internally thread-safe. peer_mutex_ only guards pointer
 * swaps (write_buffer_ / read_cache_) and fetched_link_templates_. Never held across
 * InMemoryDB calls or backend I/O.
 */
class RemoteAtomDBPeer : public AtomDB, public processor::ThreadMethod {
   public:
    RemoteAtomDBPeer(shared_ptr<AtomDB> remote_atomdb,
                     shared_ptr<AtomDB> local_persistence = nullptr,
                     const string& uid = "");
    ~RemoteAtomDBPeer();

    bool allow_nested_indexing() override;
    bool composite_type_enabled() const override;

    shared_ptr<Atom> get_atom(const string& handle) override;
    shared_ptr<Node> get_node(const string& handle) override;
    shared_ptr<Link> get_link(const string& handle) override;

    // In-memory lookups only (write_buffer + read_cache). Used by the RemoteAtomDB facade
    // to probe every peer's cache before escalating any peer to its backend.
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

    string add_atom(const atoms::Atom* atom, const atoms::Merger* merger = NULL) override;
    string add_node(const atoms::Node* node, const atoms::Merger* merger = NULL) override;
    string add_link(const atoms::Link* link, const atoms::Merger* merger = NULL) override;

    vector<string> add_atoms(const vector<atoms::Atom*>& atoms,
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

    void re_index_patterns(bool flush_patterns = true) override;

    size_t node_count() const override;
    size_t link_count() const override;
    size_t atom_count() const override;

    // Cache policy API
    void fetch(const LinkSchema& link_schema);
    void release(const LinkSchema& link_schema, bool persist = true, bool force = false);
    // Flushes write_buffer_ to local_persistence (when present) and drops both in-memory layers.
    // Legacy bool args are ignored and kept only for call-site compatibility.
    void release_cache(bool persist_to_local = true, bool persist_entire_cache = false);
    double available_ram();
    void auto_cleanup();
    void start_cleanup_thread();
    void stop_cleanup_thread();

    // ThreadMethod interface
    bool thread_one_step() override;

    const string& get_uid() const { return uid_; }
    bool is_readonly() const { return local_persistence_ == nullptr; }

    /**
     * Delegates the lookup to the remote AtomDB; local persistence is not consulted.
     * Returns an empty vector if there is no remote AtomDB or it has no matching permissions.
     */
    vector<shared_ptr<atomdb_api_types::AccessPermissionDocument>> get_access_permissions(
        const atomdb_api_types::PublicKey& public_key) const override;

   private:
    shared_ptr<InMemoryDB> write_buffer() const;
    shared_ptr<InMemoryDB> read_cache() const;
    void invalidate_fetched_templates();

    void feed_cache_from_handle_set(shared_ptr<atomdb_api_types::HandleSet> handle_set);
    void merge_handle_set(shared_ptr<atomdb_api_types::HandleSet> source,
                          shared_ptr<atomdb_api_types::HandleSetInMemory> dest,
                          set<string>& seen,
                          bool copy_metadata = false);
    void persist_atoms_to_local(const vector<shared_ptr<atoms::Atom>>& atoms);
    // Puts atoms back into the current write buffer after a failed flush so the next
    // release retries them instead of losing dirty writes.
    void restage_atoms(const vector<shared_ptr<atoms::Atom>>& atoms);

    string uid_;
    shared_ptr<InMemoryDB> write_buffer_;
    shared_ptr<InMemoryDB> read_cache_;
    shared_ptr<AtomDB> atomdb_;
    shared_ptr<AtomDB> local_persistence_;
    unordered_set<string> fetched_link_templates_;
    unique_ptr<processor::DedicatedThread> cleanup_thread_;

    // Guards pointer swaps and fetched_link_templates_. Never held across backend
    // (local_persistence_ / atomdb_) I/O. It IS held across cheap in-memory InMemoryDB
    // metadata calls (atom_count in release paths); that is safe because InMemoryDB never
    // acquires peer_mutex_, so no lock cycle exists — keep it that way.
    mutable mutex peer_mutex_;

    static constexpr double CRITICAL_RAM_THRESHOLD = 0.1;  // 10% - cleanup when below this
};

}  // namespace atomdb
