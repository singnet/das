#include "PostgresMappingStrategy.h"

#include <filesystem>
#include <string>

#include "ContextLoader.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace commons;
using namespace db_adapter;

namespace fs = std::filesystem;

// ==============================
//  Construction / destruction
// ==============================

PostgresMappingStrategy::PostgresMappingStrategy(const JsonConfig& config,
                                                 shared_ptr<PostgresConnection> conn,
                                                 shared_ptr<BoundedSharedQueue> queue)
    : DatabaseMappingStrategy(conn), config(config) {
    this->wrapper = make_shared<PostgresWrapper>(*conn, queue);
}

// ==============================
//  Public
// ============================

vector<MappingTask> PostgresMappingStrategy::create_tasks() {
    vector<string> file_paths =
        this->config.at_path("adapterdb.context_mapping_paths").get_or<vector<string>>({});

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

            auto queries_sql = ContextLoader::load_sql_queries(path);

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