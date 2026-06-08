#include "MorkMappingStrategy.h"

#include <filesystem>

#include "ContextLoader.h"
#include "Utils.h"

using namespace std;
using namespace commons;
using namespace db_adapter;

namespace fs = std::filesystem;

// ==============================
//  Construction / destruction
// ==============================

db_adapter::MorkMappingStrategy::MorkMappingStrategy(const JsonConfig& config,
                                                     shared_ptr<MorkConnection> conn,
                                                     shared_ptr<BoundedSharedQueue> queue)
    : DatabaseMappingStrategy(conn), config(config) {
    this->wrapper = make_shared<MorkWrapper>(conn, queue);
}

// ==============================
//  Public
// ============================

vector<MappingTask> MorkMappingStrategy::create_tasks() {
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

            if (ext != ".metta") {
                RAISE_ERROR("Unsupported mapping file type: " + ext + " for file: " + path);
            }

            auto queries_metta = ContextLoader::load_metta_queries(path);

            if (!queries_metta.empty()) {
                for (size_t i = 0; i < queries_metta.size(); i++) {
                    LOG_DEBUG("Query " << (i + 1) << ": " << queries_metta[i]);
                    tasks.push_back(MappingTask{"custom_query_" + to_string(i), queries_metta[i]});
                }
            }

            LOG_DEBUG(to_string(queries_metta.size()) + " queries were loaded from the query file.");
        }
    }
    return tasks;
}

void MorkMappingStrategy::execute_task(const MappingTask& task) {
    string query = "";

    if (task.context == nullopt) {
        LOG_DEBUG("Executing task: " << task.task_name << " with no specific query context");
        query = "$all";
    } else {
        LOG_DEBUG("Executing task: " << task.task_name
                                     << " with query context: " << task.context.value());
        query = task.context.value();
    }
    this->wrapper->map(query);
}
