#include "DatabaseAdapter.h"

#include <chrono>
#include <thread>

#include "AtomDBSingleton.h"
#include "BoundedSharedQueue.h"
#include "DatabaseLoader.h"
#include "DedicatedThread.h"
#include "MorkDB.h"
#include "Processor.h"
#include "Utils.h"
#include "expression_hasher.h"
#include "processor/ThreadPool.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace commons;
using namespace atomdb;
using namespace db_adapter;
using namespace processor;

string DatabaseAdapter::MONGODB_ADAPTER_COLLECTION_NAME = "database_adapter";

// ==============================
//  Construction / destruction
// ==============================

DatabaseAdapter::DatabaseAdapter(const JsonConfig& config) : config(config) {
    string atomdb_type = config.at_path("atomdb.type").get_or<string>("");

    if (atomdb_type != "morkdb") {
        Utils::error("Unsupported AtomDB type in config: " + atomdb_type +
                     ". DatabaseAdapter currently only supports 'morkdb'.");
    }

    string type = config.at_path("adapter.type").get_or<string>("");

    if (type != "postgres") {
        Utils::error("DatabaseAdapter: Unsupported database type in config: " + type);
    }

    string context_id = this->context_id();

    LOG_DEBUG("Generated contextID: " << context_id);

    this->atomdb = AtomDBSingleton::get_instance();

    if (!this->is_loaded(context_id)) {
        lock_guard<mutex> lock(mtx);
        this->sync_source_database_to_atomdb();
    }

    LOG_INFO("Context already loaded. Skipping adapter pipeline.");
}

DatabaseAdapter::~DatabaseAdapter() {}

// ==============================
//  Public
// ==============================

void DatabaseAdapter::reload() {
    lock_guard<mutex> lock(mtx);
    this->sync_source_database_to_atomdb();
}

bool DatabaseAdapter::needs_sync() const {
    
}

bool DatabaseAdapter::allow_nested_indexing() { return this->atomdb->allow_nested_indexing(); }

shared_ptr<Atom> DatabaseAdapter::get_atom(const string& handle) {
    return this->atomdb->get_atom(handle);
}

shared_ptr<Node> DatabaseAdapter::get_node(const string& handle) {
    return this->atomdb->get_node(handle);
}

shared_ptr<Link> DatabaseAdapter::get_link(const string& handle) {
    return this->atomdb->get_link(handle);
}

vector<shared_ptr<Atom>> DatabaseAdapter::get_matching_atoms(bool is_toplevel, Atom& key) {
    return this->atomdb->get_matching_atoms(is_toplevel, key);
}

shared_ptr<atomdb_api_types::HandleSet> DatabaseAdapter::query_for_pattern(
    const LinkSchema& link_schema) {
    return this->atomdb->query_for_pattern(link_schema);
}

shared_ptr<atomdb_api_types::HandleList> DatabaseAdapter::query_for_targets(const string& handle) {
    return this->atomdb->query_for_targets(handle);
}

shared_ptr<atomdb_api_types::HandleSet> DatabaseAdapter::query_for_incoming_set(const string& handle) {
    return this->atomdb->query_for_incoming_set(handle);
}

bool DatabaseAdapter::atom_exists(const string& handle) { return this->atomdb->atom_exists(handle); }

bool DatabaseAdapter::node_exists(const string& handle) { return this->atomdb->node_exists(handle); }

bool DatabaseAdapter::link_exists(const string& handle) { return this->atomdb->link_exists(handle); }

set<string> DatabaseAdapter::atoms_exist(const vector<string>& handles) {
    return this->atomdb->atoms_exist(handles);
}

set<string> DatabaseAdapter::nodes_exist(const vector<string>& handles) {
    return this->atomdb->nodes_exist(handles);
}

set<string> DatabaseAdapter::links_exist(const vector<string>& handles) {
    return this->atomdb->links_exist(handles);
}

string DatabaseAdapter::add_atom(const atoms::Atom* atom, bool throw_if_exists) {
    return this->atomdb->add_atom(atom, throw_if_exists);
}

string DatabaseAdapter::add_node(const atoms::Node* node, bool throw_if_exists) {
    return this->atomdb->add_node(node, throw_if_exists);
}

string DatabaseAdapter::add_link(const atoms::Link* link, bool throw_if_exists) {
    return this->atomdb->add_link(link, throw_if_exists);
}

vector<string> DatabaseAdapter::add_atoms(const vector<atoms::Atom*>& atoms,
                                          bool throw_if_exists,
                                          bool is_transactional) {
    return this->atomdb->add_atoms(atoms, throw_if_exists, is_transactional);
}

vector<string> DatabaseAdapter::add_nodes(const vector<atoms::Node*>& nodes,
                                          bool throw_if_exists,
                                          bool is_transactional) {
    return this->atomdb->add_nodes(nodes, throw_if_exists, is_transactional);
}

vector<string> DatabaseAdapter::add_links(const vector<atoms::Link*>& links,
                                          bool throw_if_exists,
                                          bool is_transactional) {
    return this->atomdb->add_links(links, throw_if_exists, is_transactional);
}

bool DatabaseAdapter::delete_atom(const string& handle, bool delete_link_targets) {
    return this->atomdb->delete_atom(handle, delete_link_targets);
}

bool DatabaseAdapter::delete_node(const string& handle, bool delete_link_targets) {
    return this->atomdb->delete_node(handle, delete_link_targets);
}

bool DatabaseAdapter::delete_link(const string& handle, bool delete_link_targets) {
    return this->atomdb->delete_link(handle, delete_link_targets);
}

uint DatabaseAdapter::delete_atoms(const vector<string>& handles, bool delete_link_targets) {
    return this->atomdb->delete_atoms(handles, delete_link_targets);
}

uint DatabaseAdapter::delete_nodes(const vector<string>& handles, bool delete_link_targets) {
    return this->atomdb->delete_nodes(handles, delete_link_targets);
}

uint DatabaseAdapter::delete_links(const vector<string>& handles, bool delete_link_targets) {
    return this->atomdb->delete_links(handles, delete_link_targets);
}

void DatabaseAdapter::re_index_patterns(bool flush_patterns) {
    this->atomdb->re_index_patterns(flush_patterns);
}

// ==============================
//  Private
// ==============================

bool DatabaseAdapter::is_loaded(const string& context_id) const {
    auto db = dynamic_pointer_cast<MorkDB>(this->atomdb);

    auto conn = db->get_mongo_pool()->acquire();
    auto mongodb_collection = (*conn)["das"][MONGODB_ADAPTER_COLLECTION_NAME];
    auto reply = mongodb_collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp("_id", context_id)));

    if (reply != bsoncxx::v_noabi::stdx::nullopt) {
        LOG_INFO("ContextID " << context_id << " found in AtomDB backend.");
        return true;
    } else {
        LOG_INFO("ContextID " << context_id << " NOT found in AtomDB backend.");

        bsoncxx::builder::basic::array mapped_context_array;
        vector<string> file_contents = this->get_mapping_file_contents();
        for (auto& content : file_contents) {
            LOG_DEBUG("Mapping file content:\n" << content);
            bsoncxx::builder::basic::document file_doc;
            file_doc.append(bsoncxx::v_noabi::builder::basic::kvp("content", content));
            mapped_context_array.append(file_doc.extract());
        }

        string mapped_database = this->config.at_path("adapter.type").get_or<string>("");

        auto document = bsoncxx::v_noabi::builder::basic::make_document(
            bsoncxx::v_noabi::builder::basic::kvp("_id", context_id),
            bsoncxx::v_noabi::builder::basic::kvp("mapped_database", mapped_database),
            bsoncxx::v_noabi::builder::basic::kvp("mapped_at",
                                                  bsoncxx::types::b_date(chrono::system_clock::now())),
            bsoncxx::v_noabi::builder::basic::kvp("mapped_context", mapped_context_array));

        bsoncxx::builder::stream::document filter_builder;
        filter_builder << "_id" << context_id;

        mongocxx::options::replace opts;
        opts.upsert(true);

        auto reply = mongodb_collection.replace_one(filter_builder.view(), document.view(), opts);

        if (!reply) {
            Utils::error("Failed to upsert document into MongoDB");
        }

        return false;
    }
}

string DatabaseAdapter::context_id() const {
    vector<string> file_contents = this->get_mapping_file_contents();
    string combined_content;
    for (const auto& c : file_contents) {
        combined_content += c;
    }
    return compute_hash((char*) combined_content.c_str());
}

vector<string> DatabaseAdapter::get_mapping_file_paths() const {
    string tables_file_path = this->config.at_path("adapter.context_mapping.tables").get_or<string>("");
    string query_file_path =
        this->config.at_path("adapter.context_mapping.queries_sql").get_or<string>("");

    if (tables_file_path.empty() && query_file_path.empty()) {
        Utils::error(
            "At least one of adapter.context_mapping.tables or adapter.context_mapping.queries_sql "
            "must be provided in config");
    }

    vector<string> file_paths;

    if (!tables_file_path.empty()) {
        file_paths.push_back(tables_file_path);
    }

    if (!query_file_path.empty()) {
        file_paths.push_back(query_file_path);
    }

    return file_paths;
}

vector<string> DatabaseAdapter::get_mapping_file_contents() const {
    vector<string> file_paths = this->get_mapping_file_paths();
    vector<string> contents;

    for (const auto& path : file_paths) {
        ifstream file(path);

        if (!file.is_open()) {
            Utils::error("Failed to open file for context ID generation: " + path);
        }
        stringstream buffer;
        buffer << file.rdbuf();
        contents.push_back(buffer.str());
    }

    return contents;
}

void DatabaseAdapter::sync_source_database_to_atomdb() {
    LOG_INFO("Running adapter pipeline...");

    auto queue = make_shared<BoundedSharedQueue>();

    DatabaseMappingOrchestrator db_mapping_orchestrator(this->config, queue);
    auto producer = make_shared<DedicatedThread>("producer", &db_mapping_orchestrator);

    ThreadPool pool("consumers_pool", THREAD_POOL_SIZE);
    pool.setup();
    pool.start();

    bool save_metta = true;

    LOG_INFO("Save metta: " << (save_metta ? "ENABLED" : "DISABLED"));

    MultiThreadAtomPersister consumer(queue, pool, BATCH_SIZE, save_metta);

    producer->setup();
    producer->start();

    LOG_DEBUG("Producer thread started.");

    while (true) {
        consumer.dispatch();

        if (db_mapping_orchestrator.is_finished() && !consumer.is_producer_finished()) {
            LOG_INFO("Mapping completed. Loading data into DAS.");
            producer->stop();
            consumer.set_producer_finished();
        }

        if (consumer.is_producer_finished() && queue->empty()) {
            consumer.dispatch();
            break;
        }

        if (queue->empty()) {
            Utils::sleep(5);
        }
    }

    pool.wait();
    pool.stop();

    LOG_INFO("Loading completed! Total atoms: " << consumer.get_total_count());
}
