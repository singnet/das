#pragma once

#include <Atom.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "BoundedSharedQueue.h"
#include "PostgresConnection.h"
#include "SqlWrapper.h"

#define MAX_VALUE_SIZE ((size_t) 1000)

using namespace std;
using namespace atoms;
using namespace commons;

namespace db_adapter {

/**
 * @class PostgresWrapper
 * @brief Concrete implementation of SQLWrapper for PostgreSQL using libpqxx.
 */
class PostgresWrapper : public SQLWrapper {
   public:
    /**
     * @brief Constructs a PostgresWrapper.
     *
     * @param db_conn The PostgreSQL database connection.
     * @param output_queue Optional shared queue for outputting mapped data.
     * @param mapper_type The strategy for mapping results.
     */
    PostgresWrapper(PostgresConnection& db_conn,
                    shared_ptr<BoundedSharedQueue> output_queue = nullptr,
                    MAPPER_TYPE mapper_type = MAPPER_TYPE::SQL2ATOMS);

    ~PostgresWrapper() override;

    Table get_table(const string& schema_name, const string& table_name) override;
    vector<Table> list_tables() override;
    void map_table(const Table& table,
                   const vector<string>& clauses,
                   const vector<string>& skip_columns = {},
                   bool second_level = false) override;
    void map_sql_query(const string& virtual_name, const string& raw_query) override;

   private:
    // Regex for parsing alias patterns (e.g., "AS public_feature__uniquename")
    static constexpr const char* ALIAS_PATTERN_REGEX =
        R"(\bAS\s+([a-zA-Z_][a-zA-Z0-9_]*)__([a-zA-Z_][a-zA-Z0-9_]*))";

    atomic<int> count = 0;
    mutex api_mutex;
    PostgresConnection& db_conn;
    shared_ptr<BoundedSharedQueue> output_queue;
    optional<vector<Table>> tables_cache;
    unordered_map<string, int> tables_rows_count;

    vector<string> build_columns_to_map(const Table& table, const vector<string>& skip_columns = {});
    vector<string> collect_fk_ids(const string& table_name,
                                  const string& column_name,
                                  const string& where_clause = "");
    map<string, vector<string>> extract_aliases_from_query(const string& query);
    void fetch_rows_paginated(const Table& table, const vector<string>& columns, const string& query);
    SqlRow build_sql_row(const pqxx::row& row, const Table& table, vector<string> columns);
    void log_progress(const string& table_name, int rows_count);
    double get_available_ram_ratio();
};

}  // namespace db_adapter