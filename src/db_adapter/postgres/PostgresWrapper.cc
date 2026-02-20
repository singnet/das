#include "PostgresWrapper.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <pqxx/pqxx>
#include <regex>
#include <stdexcept>
#include <string>

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;

PostgresDatabaseConnection::PostgresDatabaseConnection(const string& id,
                                                       const string& host,
                                                       int port,
                                                       const string& database,
                                                       const string& user,
                                                       const string& password)
    : DatabaseConnection(id, host, port), database(database), user(user), password(password) {}

PostgresDatabaseConnection::~PostgresDatabaseConnection() {
    if (this->is_running()) {
        this->stop();
    }
}

void PostgresDatabaseConnection::connect() {
    LOG_INFO("Connecting to PostgreSQL database at " << host << ":" << port << "...");
    try {
        string conn_str = "host=" + host + " port=" + std::to_string(port) + " dbname=" + database;
        if (!user.empty()) {
            conn_str += " user=" + user;
        }
        if (!password.empty()) {
            conn_str += " password=" + password;
        }
        this->conn = make_unique<pqxx::connection>(conn_str);
    } catch (const exception& e) {
        throw runtime_error("Could not connect to database: " + string(e.what()));
    }
}

void PostgresDatabaseConnection::disconnect() {
    if (this->conn) {
        this->conn->close();
        this->conn.reset();
    }
}

pqxx::result PostgresDatabaseConnection::execute_query(const string& query) {
    if (!this->conn || !this->conn->is_open()) {
        Utils::error("Postgres connection is not open.");
    }

    try {
        pqxx::work transaction(*this->conn);
        pqxx::result result = transaction.exec(query);
        transaction.commit();
        return result;
    } catch (const exception& e) {
        Utils::error("Error during query execution: " + string(e.what()));
    }
    return pqxx::result{};
}

// ===============================================================================================
// PostgresWrapper implementation
// ===============================================================================================

PostgresWrapper::PostgresWrapper(PostgresDatabaseConnection& db_conn,
                                 MAPPER_TYPE mapper_type,
                                 shared_ptr<SharedQueue> output_queue)
    : SQLWrapper(db_conn, mapper_type), db_conn(db_conn), output_queue(output_queue) {}

PostgresWrapper::~PostgresWrapper() {}

Table PostgresWrapper::get_table(const string& name) {
    auto tables = this->list_tables();
    for (const auto& table : tables) {
        if (table.name == name) return table;
    }
    Utils::error("Table '" + name + "' not found in the database.");
}

vector<Table> PostgresWrapper::list_tables() {
    if (tables_cache.has_value()) {
        return tables_cache.value();
    }

    string query = R"(WITH table_info AS (
        SELECT
            schemaname || '.' || tablename AS table_name,
            (SELECT reltuples::bigint FROM pg_class WHERE oid = (schemaname || '.' || tablename)::regclass) AS row_count,
            pg_tables.schemaname,
            pg_tables.tablename
        FROM
            pg_tables
        WHERE
            schemaname NOT IN ('pg_catalog', 'information_schema')
    ),
    column_info AS (
        SELECT
            table_schema || '.' || table_name AS table_name,
            string_agg(column_name, ',') AS columns
        FROM
            information_schema.columns
        WHERE
            table_schema NOT IN ('pg_catalog', 'information_schema')
        GROUP BY
            table_schema, table_name
    ),
    pk_info AS (
        SELECT
            tc.table_schema || '.' || tc.table_name AS table_name,
            string_agg(kcu.column_name, ',') AS pk_column
        FROM
            information_schema.table_constraints AS tc
            JOIN information_schema.key_column_usage AS kcu
            ON tc.constraint_name = kcu.constraint_name
            AND tc.table_schema = kcu.table_schema
        WHERE
            tc.constraint_type = 'PRIMARY KEY'
        GROUP BY
            tc.table_schema, tc.table_name
    ),
    fk_info AS (
        SELECT
            tc.table_schema || '.' || tc.table_name AS table_name,
            string_agg(kcu.column_name || '|' || ccu.table_schema || '.' || ccu.table_name, ',') AS fk_columns
        FROM
            information_schema.table_constraints AS tc
            JOIN information_schema.key_column_usage AS kcu
            ON tc.constraint_name = kcu.constraint_name
            AND tc.table_schema = kcu.table_schema
            JOIN information_schema.constraint_column_usage AS ccu
            ON ccu.constraint_name = tc.constraint_name
        WHERE
            tc.constraint_type = 'FOREIGN KEY'
        GROUP BY
            tc.table_schema, tc.table_name
    )
    SELECT
        ti.table_name,
        ti.row_count,
        ci.columns,
        COALESCE(pk.pk_column, '') AS pk_column,
        COALESCE(fk.fk_columns, '') AS fk_columns
    FROM
        table_info ti
    LEFT JOIN
        column_info ci ON ti.table_name = ci.table_name
    LEFT JOIN
        pk_info pk ON ti.table_name = pk.table_name
    LEFT JOIN
        fk_info fk ON ti.table_name = fk.table_name
    ORDER BY
        pg_total_relation_size(ti.table_name) ASC;
    )";
    auto result = db_conn.execute_query(query);
    vector<Table> tables;
    tables.reserve(result.size());

    for (const auto& row : result) {
        string table_name = row["table_name"].c_str();
        int row_count = row["row_count"].as<int>(0);
        string columns = row["columns"].c_str();
        string pk_column = row["pk_column"].c_str();
        string fk_columns = row["fk_columns"].c_str();

        Table t;
        t.name = table_name;
        t.row_count = row_count;
        t.primary_key = pk_column;
        t.column_names = columns.empty() ? vector<string>{} : Utils::split(columns, ',');
        t.foreign_keys = fk_columns.empty() ? vector<string>{} : Utils::split(fk_columns, ',');

        tables.push_back(move(t));
    }
    this->tables_cache = tables;
    return tables;
}

void PostgresWrapper::map_table(const Table& table,
                                const vector<string>& clauses,
                                const vector<string>& skip_columns,
                                bool second_level) {
    LOG_DEBUG("Mapping table: " << table.name);

    string where_clauses;

    for (size_t i = 0; i < clauses.size(); ++i) {
        if (i > 0) where_clauses += " AND ";
        where_clauses += clauses[i];
    }

    auto columns = build_columns_to_map(table, skip_columns);

    string cols_sql = Utils::join(columns, ',');
    string base_query = "SELECT " + cols_sql + " FROM " + table.name;
    if (!where_clauses.empty()) {
        base_query += " WHERE " + where_clauses;
    }
    LOG_DEBUG("Base query: " << base_query);
    this->fetch_rows_paginated(table, columns, base_query);

    if (second_level) {
        for (const auto& fk : table.foreign_keys) {
            auto parts = Utils::split(fk, '|');

            if (parts.size() != 2) {
                Utils::error("Invalid foreign key format: " + fk);
            }

            string column = parts[0];
            string ref_table_name = parts[1];

            // Collect distinct non-null foreign-key values
            auto fk_ids = this->collect_fk_ids(table.name, column, where_clauses);

            if (fk_ids.empty()) continue;

            auto ref_table = this->get_table(ref_table_name);
            auto ref_columns = this->build_columns_to_map(ref_table);

            string where_clause = ref_table.primary_key + " IN " + "(" + Utils::join(fk_ids, ',') + ")";

            string cols_sql = Utils::join(ref_columns, ',');
            string base_query = "SELECT " + cols_sql + " FROM " + ref_table.name;
            if (!where_clause.empty()) {
                base_query += " WHERE " + where_clause;
            }

            this->fetch_rows_paginated(ref_table, ref_columns, base_query);
        }
    }
}

vector<string> PostgresWrapper::build_columns_to_map(const Table& table,
                                                     const vector<string>& skip_columns) {
    for (const auto& skipo_col : skip_columns) {
        if (find(table.column_names.begin(), table.column_names.end(), skipo_col) ==
            table.column_names.end()) {
            Utils::error("Skip column '" + skipo_col + "' not found in table '" + table.name + "'");
        }
    }

    vector<string> columns_to_process = table.column_names;

    vector<string> non_primary_key_columns;
    for (const auto& col : columns_to_process) {
        if (col != table.primary_key) non_primary_key_columns.push_back(col);
    }

    if (!skip_columns.empty()) {
        vector<string> filtered_columns;
        for (const auto& col : non_primary_key_columns) {
            if (find(skip_columns.begin(), skip_columns.end(), col) == skip_columns.end()) {
                filtered_columns.push_back(col);
            }
        }
        non_primary_key_columns = move(filtered_columns);
    }

    vector<string> columns_list;
    columns_list.push_back(table.primary_key);
    columns_list.insert(
        columns_list.end(), non_primary_key_columns.begin(), non_primary_key_columns.end());

    vector<string> final_columns;
    for (const auto& item : columns_list) {
        if (!item.empty()) final_columns.push_back(item);
    }

    return final_columns;
}

vector<string> PostgresWrapper::collect_fk_ids(const string& table_name,
                                               const string& column_name,
                                               const string& where_clause) {
    vector<string> ids;

    size_t offset = 0;
    size_t limit = 10000;

    while (true) {
        string query = "SELECT " + column_name + " FROM " + table_name + " WHERE " + where_clause +
                       " LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset) + ";";
        pqxx::result rows = db_conn.execute_query(query);

        if (rows.empty()) break;

        for (const pqxx::row& row : rows) {
            auto field = row[0];
            if (!field.is_null()) {
                string value = field.c_str();
                ids.push_back(value);
            }
        }

        offset += limit;
    }
    return ids;
}

void PostgresWrapper::map_sql_query(const string& virtual_name, const string& raw_query) {
    map<string, vector<string>> table_columns_map = this->extract_aliases_from_query(raw_query);

    if (table_columns_map.empty()) {
        Utils::error("No valid aliases found in query for " + virtual_name);
    }

    map<string, Table> tables_metadata;

    // Search metadata (PK, FK, ...) of each referenced table
    // and validate that each table has its PK included in the aliases
    for (const auto& table_columns : table_columns_map) {
        string table_name = table_columns.first;
        vector<string> columns = table_columns.second;
        try {
            tables_metadata[table_name] = this->get_table(table_name);
            string pk = tables_metadata[table_name].primary_key;
            if (find(columns.begin(), columns.end(), pk) == columns.end()) {
                auto parts = Utils::split(table_name, '.');
                string schema = parts[0];
                string table = parts[1];
                Utils::error("Primary key '" + pk + "' of table '" + table_name +
                             "' must be included in SELECT aliases. Add: " + table + "." + pk + " AS " +
                             schema + "_" + table + "__" + pk);
            }
        } catch (const exception& e) {
            Utils::error("Error retrieving metadata for table '" + table_name + "': " + e.what());
        }
    }

    string base_query = Utils::trim(raw_query);

    if (!base_query.empty() && base_query.back() == ';') base_query.pop_back();

    for (const auto& table_columns : table_columns_map) {
        string table_name = table_columns.first;
        vector<string> columns = table_columns.second;
        Table table = tables_metadata[table_name];
        this->fetch_rows_paginated(table, columns, base_query);
    }
}

map<string, vector<string>> PostgresWrapper::extract_aliases_from_query(const string& query) {
    map<string, vector<string>> tables;

    regex alias_pattern(alias_pattern_regex, regex_constants::icase);

    auto matches_begin = sregex_iterator(query.begin(), query.end(), alias_pattern);
    auto matches_end = sregex_iterator();

    for (auto it = matches_begin; it != matches_end; ++it) {
        smatch match = *it;
        string table_part = match[1].str();
        string column_name = match[2].str();

        string table_name;

        size_t dot_pos = table_part.find('.');
        if (dot_pos != string::npos) {
            table_name = table_part;
        } else {
            size_t underscore_pos = table_part.find('_');
            if (underscore_pos != string::npos) {
                table_name =
                    table_part.substr(0, underscore_pos) + "." + table_part.substr(underscore_pos + 1);
            } else {
                table_name = "public." + table_part;
            }
        }

        auto& columns = tables[table_name];
        if (find(columns.begin(), columns.end(), column_name) == columns.end()) {
            columns.push_back(column_name);
        }
    }

    return tables;
}

void PostgresWrapper::fetch_rows_paginated(const Table& table,
                                           const vector<string>& columns,
                                           const string& query) {
    size_t offset = 0;
    size_t limit = 10000;

    while (true) {
        string paginated_query =
            query + " LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);
        pqxx::result rows = db_conn.execute_query(paginated_query);

        LOG_DEBUG("Executing paginated query: " << paginated_query);
        LOG_DEBUG("Fetched " << rows.size() << " rows from table " << table.name);

        if (rows.empty()) break;

        for (const auto& row : rows) {
            SqlRow sql_row = this->build_sql_row(row, table, columns);

            LOG_DEBUG("Built SqlRow for table "
                      << table.name << " with primary key: "
                      << (sql_row.primary_key ? sql_row.primary_key->value : "NULL"));
            for (const auto& field : sql_row.fields) {
                LOG_DEBUG("  Field: " << field.name << " = " << field.value);
            }

            auto output = this->mapper->map(DbInput{sql_row});

            if (this->mapper_type == MAPPER_TYPE::SQL2ATOMS) {
                auto atoms = get<vector<Atom*>>(output);

                LOG_DEBUG("Atoms count: " << atoms.size());

                unique_lock<mutex> lock(this->api_mutex);
                for (const auto& atom : atoms) {
                    this->output_queue->enqueue((void*) atom);
                }
            } else {
                auto metta_expressions = get<vector<string>>(output);
                LOG_DEBUG("Metta Expressions count: " << metta_expressions.size());
                // WIP - save metta expressions to file
            }

            LOG_DEBUG("Mapper HandleTrie size: " << this->mapper->handle_trie_size());
        }

        offset += limit;
    }
}

SqlRow PostgresWrapper::build_sql_row(const pqxx::row& row, const Table& table, vector<string> columns) {
    SqlRow sql_row;
    sql_row.table_name = table.name;
    sql_row.primary_key = ColumnValue{columns[0], row[0].c_str()};

    for (size_t i = 1; i < columns.size() && i < row.size(); i++) {
        string col = columns[i];
        auto field = row[i];

        if (field.is_null()) continue;

        string value = field.c_str();

        // datetime → SKIP
        // YYYY-MM-DD HH:MM:SS...
        if (value.size() >= 19 && value[4] == '-' && value[7] == '-') continue;

        if (value.empty()) {
            continue;
        } else if (value.size() > MAX_VALUE_SIZE) {
            continue;
        }

        string column_name = col;
        for (const auto& fk : table.foreign_keys) {
            if (fk.find(col) != string::npos) {
                column_name = fk;
                break;
            }
        }
        sql_row.add_field(column_name, value);
    }
    return sql_row;
}
