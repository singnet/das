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
#include "Utils.h"

using namespace std;
using namespace db_adapter;

namespace atomdb {

enum class AdapterDbType { Postgres };

inline AdapterDbType parse_adapter_db_type(const std::string& value) {
    if (value == "postgres") return AdapterDbType::Postgres;
    RAISE_ERROR("Unsupported adapterdb.type: " + value);
}

class AdapterDB : public AtomDB {
   public:
    explicit AdapterDB(const JsonConfig& config);
    AdapterDB(const JsonConfig& config, std::shared_ptr<AtomDB> backend);  // for testing
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

    size_t node_count() const override;
    size_t link_count() const override;
    size_t atom_count() const override;

   private:
    JsonConfig config;
    shared_ptr<AtomDB> atomdb_backend;
    atomic<bool> backend_ready{false};
    mutex mtx;
    mongocxx::pool* mongodb_pool;

    void initialize(bool skip_atomdb_backend_empty = false);

    /**
     * @brief Use a persistence layer to track loaded contexts and store metadata about the source
     * database and mapping files.
     */
    void persistence_setup();

    /**
     * @brief Initializes the AtomDB backend according to the configuration.
     */
    void atomdb_backend_setup();

    bool is_backend_ready() const;

    void ensure_backend_ready() const;

    /**
     * @brief a string that identifies this mapping context (a hash). Used to decide whether the backend
     * already holds this data.
     */
    string context_id() const;

    bool is_context_persisted(const string& context_id) const;

    void persist_mapping_context(const string& context_id);

    /**
     * @brief Reads the mapping files specified in the configuration and returns their contents as a
     * vector of strings.
     */
    vector<string> get_mapping_file_contents() const;

    /**
     * @brief Runs the full adapter pipeline (orchestrator -> persister) synchronously.
     */
    void synchronous_source_database_to_atomdb();
};

}  // namespace atomdb
