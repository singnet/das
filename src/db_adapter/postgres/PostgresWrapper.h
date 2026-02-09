#pragma once

#include <Atom.h>

#include <memory>
#include <optional>
#include <pqxx/pqxx>
#include <regex>
#include <string>
#include <vector>

#include "DBWrapper.h"

#define MAX_VALUE_SIZE ((size_t) 1000)

using namespace std;
using namespace atoms;
using namespace commons;

namespace db_adapter {

class PostgresDBConnection : public DBConnection {
   public:
    PostgresDBConnection(const string& id,
                         const string& host,
                         int port,
                         const string& database,
                         const string& user,
                         const string& password);
    ~PostgresDBConnection() override;

    void connect() override;
    void disconnect() override;
    pqxx::result execute_query(const string& query);

   protected:
    unique_ptr<pqxx::connection> conn;
    string database;
    string user;
    string password;
};

/**
 * @class PostgresWrapper
 * @brief Concrete implementation of SQLWrapper for PostgreSQL using libpqxx.
 */
class PostgresWrapper : public SQLWrapper<PostgresDBConnection> {
   public:
    /**
     * @brief Constructs a PostgresWrapper.
     *
     * @param host The hostname or IP address.
     * @param port The port number.
     * @param database The database name (default: "postgres").
     * @param user The username.
     * @param password The password.
     * @param mapper_type The strategy for mapping results.
     */
    PostgresWrapper(const string& host,
                    int port,
                    const string& database = "postgres",
                    const string& user = "postgres",
                    const string& password = "postgres",
                    MAPPER_TYPE mapper_type = MAPPER_TYPE::SQL2ATOMS);

    ~PostgresWrapper() override;

    // void disconnect() override;
    Table get_table(const string& name) override;
    vector<Table> list_tables() override;
    void map_table(const Table& table,
                   const vector<string>& clauses,
                   const vector<string>& skip_columns = {},
                   bool second_level = false) override;
    void map_sql_query(const string& virtual_name, const string& raw_query) override;
    // pqxx::result execute_query(const string& query);

   protected:
    // unique_ptr<pqxx::connection> connect() override;
    // Regex for parsing alias patterns (e.g., "AS public_feature__uniquename")
    const string alias_pattern_regex = R"(\bAS\s+([a-zA-Z_][a-zA-Z0-9_]*)__([a-zA-Z_][a-zA-Z0-9_]*))";

   private:
    // string host;
    // int port;
    // string database;
    // string user;
    // string password;

    // Store tables in cache to avoid repeated database queries.
    optional<vector<Table>> tables_cache;
    vector<string> build_columns_to_map(const Table& table, const vector<string>& skip_columns = {});
    vector<string> collect_fk_ids(const string& table_name,
                                  const string& column_name,
                                  const string& where_clause = "");
    map<string, vector<string>> extract_aliases_from_query(const string& query);
    void fetch_rows_paginated(const Table& table, const vector<string>& columns, const string& query);
    SqlRow build_sql_row(const pqxx::row& row, const Table& table, vector<string> columns);
};

}  // namespace db_adapter