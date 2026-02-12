#pragma once

#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "DataMapper.h"
#include "DataTypes.h"
#include "DatabaseConnection.h"

using namespace std;
using namespace db_adapter;
using namespace commons;

namespace db_adapter {

/**
 * @class DatabaseWrapper
 * @brief Generic interface for a database connection wrapper.
 */
class DatabaseWrapper {
   public:
    DatabaseWrapper(shared_ptr<Mapper> mapper, MAPPER_TYPE mapper_type);
    virtual ~DatabaseWrapper() = default;

    unsigned int mapper_handle_trie_size();

   protected:
    unique_ptr<DatabaseConnection> db_client;
    shared_ptr<Mapper> mapper;
    MAPPER_TYPE mapper_type;
};

/**
 * @class SQLWrapper
 * @brief Specialization of DatabaseWrapper for SQL-based databases.
 */
class SQLWrapper : public DatabaseWrapper {
   public:
    explicit SQLWrapper(MAPPER_TYPE mapper_type);
    virtual ~SQLWrapper() = default;

    /**
     * @brief Retrieves schema information for a specific table.
     */
    virtual Table get_table(const string& name) = 0;

    /**
     * @brief Lists all tables in the database.
     */
    virtual vector<Table> list_tables() = 0;

    /**
     * @brief Maps a table's data to the target format.
     *
     * @param table The table metadata.
     * @param clauses Additional SQL WHERE clauses.
     * @param skip_columns Columns to exclude from mapping.
     * @param second_level Boolean flag for recursive/depth mapping logic.
     */
    virtual void map_table(const Table& table,
                           const vector<string>& clauses,
                           const vector<string>& skip_columns,
                           bool second_level) = 0;

    /**
     * @brief Executes a raw SQL query and maps the result.
     */
    virtual void map_sql_query(const string& virtual_name, const string& raw_query) = 0;
};
}  // namespace db_adapter