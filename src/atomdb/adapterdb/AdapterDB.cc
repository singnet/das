#include "AdapterDB.h"

#include <chrono>
#include <thread>

#include "BoundedSharedQueue.h"
#include "DatabaseLoader.h"
#include "DedicatedThread.h"
#include "MongoInitializer.h"
#include "MorkDB.h"
#include "Processor.h"
#include "RedisMongoDB.h"
#include "RemoteAtomDB.h"
#include "Utils.h"
#include "expression_hasher.h"
#include "processor/ThreadPool.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace commons;
using namespace db_adapter;
using namespace processor;

string AdapterDB::MONGODB_ADAPTER_COLLECTION_NAME = "adapterdb";

// ==============================
//  Construction / destruction
// ==============================

AdapterDB::AdapterDB(const JsonConfig& config) : config(config) {
    string type = config.at_path("adapter.type").get_or<string>("");

    if (type != "postgres") {
        Utils::error("AdapterDB: Unsupported database type in config: " + type);
    }

    // Use MongoDB to track loaded contexts and store metadata about the source database and mapping
    // files
    this->mongodb_setup();

    this->get_atomdb_backend();

    if (!this->is_loaded(this->context_id())) {
        if (!this->is_populated()) {
            LOG_INFO("AtomDB backend is empty. Populating AtomDB backend with source database data.");
            lock_guard<mutex> lock(mtx);
            this->sync_source_database_to_atomdb();
        } else {
            Utils::error("AtomDB backend already populated. Skipping initial population.");
        }
    } else {
        LOG_INFO("Context already loaded. Skipping adapter pipeline.");
    }
}

AdapterDB::~AdapterDB() {}

// ==============================
//  Public
// ==============================

void AdapterDB::reload() {
    lock_guard<mutex> lock(mtx);
    this->sync_source_database_to_atomdb();
}

bool AdapterDB::needs_sync() const {
    Utils::error("needs_sync() is not implemented yet.");
    return false;
}

bool AdapterDB::allow_nested_indexing() { return this->atomdb_backend->allow_nested_indexing(); }

shared_ptr<Atom> AdapterDB::get_atom(const string& handle) {
    return this->atomdb_backend->get_atom(handle);
}

shared_ptr<Node> AdapterDB::get_node(const string& handle) {
    return this->atomdb_backend->get_node(handle);
}

shared_ptr<Link> AdapterDB::get_link(const string& handle) {
    return this->atomdb_backend->get_link(handle);
}

vector<shared_ptr<Atom>> AdapterDB::get_matching_atoms(bool is_toplevel, Atom& key) {
    return this->atomdb_backend->get_matching_atoms(is_toplevel, key);
}

shared_ptr<atomdb_api_types::HandleSet> AdapterDB::query_for_pattern(const LinkSchema& link_schema) {
    return this->atomdb_backend->query_for_pattern(link_schema);
}

shared_ptr<atomdb_api_types::HandleList> AdapterDB::query_for_targets(const string& handle) {
    return this->atomdb_backend->query_for_targets(handle);
}

shared_ptr<atomdb_api_types::HandleSet> AdapterDB::query_for_incoming_set(const string& handle) {
    return this->atomdb_backend->query_for_incoming_set(handle);
}

bool AdapterDB::atom_exists(const string& handle) { return this->atomdb_backend->atom_exists(handle); }

bool AdapterDB::node_exists(const string& handle) { return this->atomdb_backend->node_exists(handle); }

bool AdapterDB::link_exists(const string& handle) { return this->atomdb_backend->link_exists(handle); }

set<string> AdapterDB::atoms_exist(const vector<string>& handles) {
    return this->atomdb_backend->atoms_exist(handles);
}

set<string> AdapterDB::nodes_exist(const vector<string>& handles) {
    return this->atomdb_backend->nodes_exist(handles);
}

set<string> AdapterDB::links_exist(const vector<string>& handles) {
    return this->atomdb_backend->links_exist(handles);
}

string AdapterDB::add_atom(const atoms::Atom* atom, bool throw_if_exists) {
    return this->atomdb_backend->add_atom(atom, throw_if_exists);
}

string AdapterDB::add_node(const atoms::Node* node, bool throw_if_exists) {
    return this->atomdb_backend->add_node(node, throw_if_exists);
}

string AdapterDB::add_link(const atoms::Link* link, bool throw_if_exists) {
    return this->atomdb_backend->add_link(link, throw_if_exists);
}

vector<string> AdapterDB::add_atoms(const vector<atoms::Atom*>& atoms,
                                    bool throw_if_exists,
                                    bool is_transactional) {
    return this->atomdb_backend->add_atoms(atoms, throw_if_exists, is_transactional);
}

vector<string> AdapterDB::add_nodes(const vector<atoms::Node*>& nodes,
                                    bool throw_if_exists,
                                    bool is_transactional) {
    return this->atomdb_backend->add_nodes(nodes, throw_if_exists, is_transactional);
}

vector<string> AdapterDB::add_links(const vector<atoms::Link*>& links,
                                    bool throw_if_exists,
                                    bool is_transactional) {
    return this->atomdb_backend->add_links(links, throw_if_exists, is_transactional);
}

bool AdapterDB::delete_atom(const string& handle, bool delete_link_targets) {
    return this->atomdb_backend->delete_atom(handle, delete_link_targets);
}

bool AdapterDB::delete_node(const string& handle, bool delete_link_targets) {
    return this->atomdb_backend->delete_node(handle, delete_link_targets);
}

bool AdapterDB::delete_link(const string& handle, bool delete_link_targets) {
    return this->atomdb_backend->delete_link(handle, delete_link_targets);
}

uint AdapterDB::delete_atoms(const vector<string>& handles, bool delete_link_targets) {
    return this->atomdb_backend->delete_atoms(handles, delete_link_targets);
}

uint AdapterDB::delete_nodes(const vector<string>& handles, bool delete_link_targets) {
    return this->atomdb_backend->delete_nodes(handles, delete_link_targets);
}

uint AdapterDB::delete_links(const vector<string>& handles, bool delete_link_targets) {
    return this->atomdb_backend->delete_links(handles, delete_link_targets);
}

void AdapterDB::re_index_patterns(bool flush_patterns) {
    this->atomdb_backend->re_index_patterns(flush_patterns);
}

// ==============================
//  Private
// ==============================

void AdapterDB::get_atomdb_backend() {
    auto atomdb_backend_config =
        this->config.at_path("adapter.atomdb_backend").get_or<JsonConfig>(JsonConfig());
    string atomdb_backend_type = atomdb_backend_config.at_path("type").get_or<string>("");
    if (atomdb_backend_type == "morkdb") {
        this->atomdb_backend = shared_ptr<AtomDB>(new MorkDB("", atomdb_backend_config));
    } else if (atomdb_backend_type == "redismongodb") {
        this->atomdb_backend = shared_ptr<AtomDB>(new RedisMongoDB("", false, atomdb_backend_config));
    } else if (atomdb_backend_type == "remotedb") {
        this->atomdb_backend = shared_ptr<AtomDB>(new RemoteAtomDB(atomdb_backend_config));
    } else {
        Utils::error("Invalid AtomDB type: " + atomdb_backend_type);
    }
}

bool AdapterDB::is_loaded(const string& context_id) const {
    auto mongodb_config = this->config.at_path("adapter.mongodb").get_or<string>("");
    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)["das"][MONGODB_ADAPTER_COLLECTION_NAME];
    auto reply = mongodb_collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp("_id", context_id)));

    if (reply != bsoncxx::v_noabi::stdx::nullopt) {
        LOG_INFO("ContextID " << context_id << " found.");
        return true;
    } else {
        LOG_INFO("ContextID " << context_id << " NOT found.");

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

bool AdapterDB::is_populated() const {
    auto conn = this->mongodb_pool->acquire();
    auto db = (*conn)["das"];

    vector<string> collection_names = db.list_collection_names();

    for (const auto& coll_name : collection_names) {
        if (coll_name == MONGODB_ADAPTER_COLLECTION_NAME) continue;
        auto collection = db[coll_name];
        if (collection.estimated_document_count() > 0) {
            LOG_INFO("Database populated. Found data in collection: " << coll_name);
            return true;
        }
    }
    return false;
}

void AdapterDB::mongodb_setup() {
    string address = config.at_path("adapter.mongodb.endpoint").get<string>();
    string user = config.at_path("adapter.mongodb.username").get<string>();
    string password = config.at_path("adapter.mongodb.password").get<string>();

    if (address.empty() || address == ":" || user.empty() || password.empty()) {
        Utils::error(
            "Invalid MongoDB configuration: need non-empty address, username, and password. "
            "Set atomdb.adapter.mongodb in JsonConfig (endpoint or hostname/port, username, password).");
    }

    string url = "mongodb://" + user + ":" + password + "@" + address;

    MongoInitializer::initialize();

    try {
        auto uri = mongocxx::uri{url};
        this->mongodb_pool = new mongocxx::pool(uri);

        auto conn = this->mongodb_pool->acquire();
        auto mongodb = (*conn)["das"];
        const auto ping_cmd =
            bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1));
        mongodb.run_command(ping_cmd.view());

        LOG_DEBUG("Connected to MongoDB at " << address);
    } catch (const exception& e) {
        Utils::error(e.what());
    }
}

string AdapterDB::context_id() const {
    vector<string> file_contents = this->get_mapping_file_contents();
    string combined_content;
    for (const auto& c : file_contents) {
        combined_content += c;
    }
    return compute_hash((char*) combined_content.c_str());
}

vector<string> AdapterDB::get_mapping_file_contents() const {
    vector<string> file_paths =
        this->config.at_path("adapter.context_mapping_paths").get_or<vector<string>>({});

    if (file_paths.empty()) {
        Utils::error("adapter.context_mapping_paths is not defined in config or is empty");
    }

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

void AdapterDB::sync_source_database_to_atomdb() {
    LOG_INFO("Start syncing source database to AtomDB.");

    auto queue = make_shared<BoundedSharedQueue>();

    DatabaseMappingOrchestrator db_mapping_orchestrator(this->config, queue);
    auto producer = make_shared<DedicatedThread>("producer", &db_mapping_orchestrator);

    ThreadPool pool("consumers_pool", THREAD_POOL_SIZE);
    pool.setup();
    pool.start();

    bool save_metta =
        this->config.at_path("adapter.export_metta_on_mapping.enabled").get_or<bool>(false);
    string metta_output_dir =
        this->config.at_path("adapter.export_metta_on_mapping.output_dir").get_or<string>("");

    LOG_INFO("Save metta: " << (save_metta ? "ENABLED" : "DISABLED"));

    MultiThreadAtomPersister consumer(
        queue, pool, this->atomdb_backend, BATCH_SIZE, save_metta, metta_output_dir);

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
