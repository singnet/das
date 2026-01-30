#pragma once

#include <Atom.h>

#include <memory>
#include <optional>
#include <pqxx/pqxx>
#include <string>

#include "DBWrapper.h"

#define MAX_VALUE_SIZE ((size_t) 1000)

using namespace std;
using namespace db_adapter_types;
using namespace atoms;
using namespace commons;

namespace db_adapter {

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
    PostgresWrapper(const string& host,
                    int port,
                    const string& database = "postgres",
                    const string& user = "postgres",
                    const string& password = "postgres",
                    MAPPER_TYPE mapper_type = MAPPER_TYPE::SQL2ATOMS);

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
                   const vector<string>& skip_columns = {},
                   bool second_level = false) override;

   private:
    string host;
    int port;
    string database;
    string user;
    string password;
    optional<vector<Table>> tables_cache;

    pqxx::result execute_query(const string& query);
    vector<string> build_columns_to_map(const Table& table, const vector<string>& skip_columns = {});
    void process_paginated(const Table& table,
                           const vector<string>& columns,
                           const string& where_clauses);
    vector<Atom*> map_row_2_atoms(const pqxx::row& row, const Table& table, vector<string> columns);
    SqlRow build_sql_row(const pqxx::row& row, const Table& table, vector<string> columns);
    vector<string> collect_fk_ids(const string& table_name,
                                  const string& column_name,
                                  const string& where_clause = "");
};

}  // namespace db_adapter