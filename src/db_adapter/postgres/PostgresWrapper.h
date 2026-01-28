#pragma once
#include <DBWrapper.h>

#include <memory>
#include <optional>
#include <pqxx/pqxx>
#include <string>

using namespace std;

/** * @class PostgresWrapper
 * @brief A class that provides a high-level interface to interact with a PostgreSQL database.
 *
 * The PostgresWrapper is responsible for managing the connection to a PostgreSQL database,
 * building and executing SQL queries. It encapsulates the connection parameters and provides
 * a simple interface for database operations.
 */
class PostgresWrapper : public SQLWrapper<pqxx::connection> {
   public:
    /**
     * @brief Constructs a PostgresWrapper with the specified connection parameters.
     *
     * @param host The hostname or IP address of the PostgreSQL server.
     * @param port The port number of the PostgreSQL server.
     * @param database The name of the database to connect to (default: "postgres").
     * @param user The username for authentication.
     * @param password The password for authentication.
     */
    PostgresWrapper(
        const string& host,
        int port,
        const string& database = "postgres",
        const string& user = "postgres",
        const string& password = "postgres",
        db_adapter_types::MAPPER_TYPE mapper_type = db_adapter_types::MAPPER_TYPE::SQL2ATOMS);

    ~PostgresWrapper() override;

    /**
     * @brief Establishes a connection to the PostgreSQL database.
     *
     * @return A unique pointer to a pqxx::connection object.
     * @throws runtime_error if the connection fails.
     */
    unique_ptr<pqxx::connection> connect() override;
    void disconnect() override;
    Table get_table(const string& name) override;
    vector<Table> list_tables() override;
    void map_table(const Table& table,
                   const vector<string>& clauses,
                   const vector<string>* skip_columns,
                   bool second_level,
                   const function<void(const vector<Atom>&)>& on_row) override;

   private:
    string host;
    int port;
    string database;
    string user;
    string password;
    optional<vector<Table>> tables_cache;

    /**
     * @brief Builds the connection string from the stored parameters.
     *
     * @return A PostgreSQL connection string.
     */
    string build_connection_string() const;

    pqxx::result execute_query(const string& query);
    vector<string> build_columns_to_map(const Table& table,
                        const vector<string>* map_columns,
                        const vector<string>* skip_columns);
    void process_pagineted(const Table& table,
                           const vector<string>& columns,
                           const string& where_clauses);
    vector<Atom*> map_row_2_atoms()
    
};
