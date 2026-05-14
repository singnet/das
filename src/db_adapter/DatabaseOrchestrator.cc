#include "DatabaseOrchestrator.h"

#include <filesystem>

#include "ContextLoader.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace std;
using namespace commons;
using namespace atoms;
using namespace db_adapter;

namespace fs = std::filesystem;

// ==============================
//  Construction / destruction
// ==============================

DatabaseMappingOrchestrator::DatabaseMappingOrchestrator(const JsonConfig& config,
                                                         shared_ptr<BoundedSharedQueue> output_queue) {
    this->database_setup(config, output_queue);
    this->tasks = this->strategy->create_tasks(config);
}

DatabaseMappingOrchestrator::~DatabaseMappingOrchestrator() { this->db_conn->stop(); }

// ==============================
//  Public
// ==============================

bool DatabaseMappingOrchestrator::thread_one_step() {
    LOG_DEBUG("DatabaseMappingOrchestrator thread_one_step called. Current task index: "
              << this->current_task);
    if (this->current_task >= this->tasks.size()) {
        this->db_conn->stop();
        return false;
    }

    if (!this->initialized) {
        this->db_conn->setup();
        this->db_conn->start();
        this->initialized = true;
    }

    auto& task = this->tasks[this->current_task];

    LOG_DEBUG("Processing task " << task.task_name);

    this->strategy->execute_task(task);

    this->current_task++;
    this->finished = (this->current_task >= this->tasks.size());
    return !this->finished;
}

bool DatabaseMappingOrchestrator::is_finished() const { return this->finished; }

// ==============================
//  Private
// ==============================

void DatabaseMappingOrchestrator::database_setup(const JsonConfig& config,
                                                 shared_ptr<BoundedSharedQueue> output_queue) {
    string adapterdb_type = config.at_path("adapterdb.type").get_or<string>("");

    // TODO: In the future we can implement a factory pattern here to support more database types
    if (adapterdb_type == "postgres") {
        string host = config.at_path("adapterdb.database_credentials.host").get<string>();
        uint port = config.at_path("adapterdb.database_credentials.port").get<uint>();
        string username = config.at_path("adapterdb.database_credentials.username").get<string>();
        string password = config.at_path("adapterdb.database_credentials.password").get<string>();
        string database = config.at_path("adapterdb.database_credentials.database").get<string>();
        auto conn = make_unique<PostgresDatabaseConnection>(
            "psql-conn", host, port, database, username, password);
        auto postgres_wrapper = make_unique<PostgresWrapper>(*conn, output_queue);
        auto postgres_strategy = make_unique<PostgresMappingStrategy>(move(postgres_wrapper));
        this->db_conn = move(conn);
        this->strategy = move(postgres_strategy);
    } else {
        RAISE_ERROR("Unsupported adapter type: " + adapterdb_type);
    }
}

PostgresMappingStrategy::PostgresMappingStrategy(unique_ptr<PostgresWrapper> wrapper)
    : wrapper(move(wrapper)) {}

vector<MappingTask> PostgresMappingStrategy::create_tasks(const JsonConfig& config) {
    vector<string> file_paths =
        config.at_path("adapterdb.context_mapping_paths").get_or<vector<string>>({});

    vector<MappingTask> tasks;

    if (file_paths.empty()) {
        LOG_INFO(
            "No context mapping files specified in config at adapterdb.context_mapping_paths. The "
            "entire database will be mapped.");
        tasks.push_back(MappingTask{"map_all_database", nullopt});
    } else {
        for (const auto& path : file_paths) {
            fs::path p(path);
            string ext = p.extension().string();

            if (ext != ".sql") {
                RAISE_ERROR("Unsupported mapping file type: " + ext + " for file: " + path);
            }

            auto queries_sql = ContextLoader::load_query_file(path);

            if (!queries_sql.empty()) {
                for (size_t i = 0; i < queries_sql.size(); i++) {
                    LOG_DEBUG("Query " << (i + 1) << ": " << queries_sql[i]);
                    tasks.push_back(MappingTask{"custom_query_" + to_string(i), queries_sql[i]});
                }
            }

            LOG_DEBUG(to_string(queries_sql.size()) + " queries were loaded from the query file.");
        }
    }
    return tasks;
}

void PostgresMappingStrategy::execute_task(const MappingTask& task) {
    if (task.context == nullopt) {
        auto tables = this->wrapper->list_tables();

        if (tables.empty()) {
            RAISE_ERROR("No tables found in the database.");
        }

        for (const auto& table : tables) {
            this->wrapper->map_table(table, vector<string>{}, vector<string>{}, false);
        }
    } else {
        this->wrapper->map_sql_query(task.task_name, task.context.value());
    }
}