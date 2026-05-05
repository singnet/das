#pragma once

#include <atomic>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/json.hpp>
#include <memory>
#include <mongocxx/client.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/pool.hpp>
#include <mutex>
#include <string>
#include <vector>

#include "AtomDB.h"
#include "DatabaseTypes.h"
#include "JsonConfig.h"

using namespace std;
using namespace db_adapter;

namespace atomdb {

/**
 * @brief A fully functional AtomDB backed by a relational database.
 *
 * During construction, it is checked whether the context defined by `config` has already been loaded
 * into the AtomDB backend. If so, the object is ready for immediate use. Otherwise, the adapter
 * pipeline runs synchronously—fetching, mapping, and persisting the Atoms—and the constructor is blocked
 * until this is completed.
 *
 * After construction the caller can use the AtomDB API normally.
 *
 * Required fields in config:
 *  - adapter.host / port / username / password / database -> source DB connection
 *  - adapter.context_mapping.tables -> path to a JSON file listing tables to map
 *  - adapter.context_mapping.queries_sql -> path to a SQL file with custom queries
 *
 * (At least one of the two mapping fields must be present.)
 *
 * @param config     JSON configuration describing the source DB and mapping context.
 */
class AdapterDB : public AtomDB {
   public:
    explicit AdapterDB(const JsonConfig& config);
    ~AdapterDB() override;

    static string MONGODB_ADAPTER_COLLECTION_NAME;

    /**
     * @brief Unconditionally re-runs the adapter pipeline against the source database.
     */
    void reload();

    /**
     * @brief Returns true if the source database contains data not yet in the backend.
     * @todo This method is not implemented yet.
     */
    bool needs_sync() const;

    // ------------------------------------------------------------------
    // AtomDB API
    // ------------------------------------------------------------------

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

   private:
    /**
     * @brief Returns true if the context is already loaded in the AtomDB backend.
     */
    bool is_loaded(const string& context_id) const;

    /**
     * @brief Returns true if the database contains any data.
     */
    bool is_populated() const;

    /**
     * Produces a string that identifies this mapping context (a hash)
     * Used to decide whether the backend already holds this data.
     */
    string context_id() const;

    vector<string> get_mapping_file_contents() const;
    void mongodb_setup();
    void get_atomdb_backend();

    /**
     * Runs the full adapter pipeline (orchestrator -> persister) synchronously.
     */
    void sync_source_database_to_atomdb();

    JsonConfig config;
    shared_ptr<AtomDB> atomdb_backend;
    mutex mtx;
    mongocxx::pool* mongodb_pool;
};

}  // namespace atomdb
