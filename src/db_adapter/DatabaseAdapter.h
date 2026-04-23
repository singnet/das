#pragma once

#include <memory>
#include <string>
#include <vector>

#include "DatabaseTypes.h"
#include "JsonConfig.h"

using namespace std;

namespace db_adapter {

/**
 * @param config JSON configuration for the adapter
 * @param save_metta Generate and save Metta expressions for mapped Atoms (optional; default: false)
 *
 * Required fields in config:
 *  - adapter.host, adapter.port, adapter.username, adapter.password, adapter.database: connection
 * info for the source database
 *  - adapter.context_mapping.tables: path to a JSON file defining tables to map
 *  - adapter.context_mapping.queries_sql: path to a SQL file with custom queries to map
 */
void run_database_adapter(const JsonConfig& config, bool save_metta = false);

}  // namespace db_adapter