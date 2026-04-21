#pragma once

#include <memory>
#include <string>
#include <vector>

#include "DatabaseTypes.h"
#include "JsonConfig.h"

using namespace std;

namespace db_adapter {

class DatabaseAdapter {
   public:
    /**
     * @param config JSON configuration for the adapter
     * @param mapper_type Type of mapper to use (default is SQL2ATOMS)
     *
     * Required fields in config:
     *  - adapter.host, adapter.port, adapter.username, adapter.password, adapter.database: connection
     * info for the source database
     *  - adapter.context_mapping.tables: path to a JSON file defining tables to map
     *  - adapter.context_mapping.queries_sql: path to a SQL file with custom queries to map
     */
    DatabaseAdapter(const JsonConfig& config, MAPPER_TYPE mapper_type = MAPPER_TYPE::SQL2ATOMS);
    ~DatabaseAdapter() = default;

    /**
     * Run the adapter: perform mapping and load data into DAS. This method blocks until the process is
     * complete.
     */
    void run();

   private:
    JsonConfig config;
    MAPPER_TYPE mapper_type;
};

}  // namespace db_adapter