#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "DataMapper.h"

using namespace std;
using namespace db_adapter;

namespace db_adapter {

/**
 * @class DatabaseWrapper
 * @brief Generic interface for a database connection wrapper.
 *
 * @tparam ConnT The underlying connection object type (e.g., pqxx::connection).
 */
template <typename ConnT>
class DatabaseWrapper {
   public:
    explicit DatabaseWrapper(shared_ptr<Mapper> mapper, MAPPER_TYPE mapper_type)
        : mapper(move(mapper)), mapper_type(mapper_type) {}
    virtual ~DatabaseWrapper() = default;

    /**
     * @brief Establishes connection to the database.
     */
    virtual unique_ptr<ConnT> connect() = 0;

    /**
     * @brief Closes the connection.
     */
    virtual void disconnect() = 0;

   protected:
    unique_ptr<ConnT> db_client;
    shared_ptr<Mapper> mapper;
    MAPPER_TYPE mapper_type;
};

/**
 * @class SQLWrapper
 * @brief Specialization of DatabaseWrapper for SQL-based databases.
 */
template <typename ConnT>
class SQLWrapper : public DatabaseWrapper<ConnT> {
   public:
    explicit SQLWrapper(MAPPER_TYPE mapper_type)
        : DatabaseWrapper<ConnT>(create_mapper(mapper_type), mapper_type) {}
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

   private:
    // Factory method for creating the specific mapper strategy
    static shared_ptr<Mapper> create_mapper(MAPPER_TYPE mapper_type) {
        switch (mapper_type) {
            case MAPPER_TYPE::SQL2METTA:
                return make_shared<SQL2MettaMapper>();
            case MAPPER_TYPE::SQL2ATOMS:
                return make_shared<SQL2AtomsMapper>();
            default:
                throw invalid_argument("Unknown mapper type");
        }
    }
};
}  // namespace db_adapter