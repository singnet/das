#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <mutex>
#include <vector>

#include "DataMapper.h"
#include "Processor.h"

using namespace std;
using namespace db_adapter;
using namespace commons;
using namespace processor;

namespace db_adapter {

class DBConnection : public Processor {
   public:
    DBConnection(const string& id, const string& host, int port);
    ~DBConnection() override;

    virtual void setup() override;
    virtual void start() override;
    virtual void stop() override;

    virtual void connect() = 0;
    virtual void disconnect() = 0;

    bool is_connected() const;

   protected:
    string host;
    int port;

   private:
    bool connected;
    mutex connection_mutex;
};

/**
 * @class DatabaseWrapper
 * @brief Generic interface for a database connection wrapper.
 *
 * @tparam ConnT The underlying connection object type (e.g., pqxx::connection).
 */
template <typename ConnectionType>
class DatabaseWrapper {
   public:
    explicit DatabaseWrapper(shared_ptr<Mapper> mapper, MAPPER_TYPE mapper_type)
        : mapper(move(mapper)), mapper_type(mapper_type) {}
    virtual ~DatabaseWrapper() = default;

    unsigned int mapper_handle_trie_size() { return mapper->handle_trie_size(); }

   protected:
    unique_ptr<ConnectionType> db_client;
    shared_ptr<Mapper> mapper;
    MAPPER_TYPE mapper_type;
};

/**
 * @class SQLWrapper
 * @brief Specialization of DatabaseWrapper for SQL-based databases.
 */
template <typename ConnectionType>
class SQLWrapper : public DatabaseWrapper<ConnectionType> {
   public:
    explicit SQLWrapper(MAPPER_TYPE mapper_type)
        : DatabaseWrapper<ConnectionType>(create_mapper(mapper_type), mapper_type) {}
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