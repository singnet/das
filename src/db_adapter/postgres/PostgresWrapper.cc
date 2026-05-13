#include "PostgresWrapper.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <pqxx/pqxx>
#include <queue>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_set>

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;

// ==============================
//  Construction / destruction
// ==============================

PostgresDatabaseConnection::PostgresDatabaseConnection(const string& id,
                                                       const string& host,
                                                       int port,
                                                       const string& database,
                                                       const string& user,
                                                       const string& password)
    : DatabaseConnection(id, host, port), database(database), user(user), password(password) {}

PostgresDatabaseConnection::~PostgresDatabaseConnection() {
    this->close_cursor();
    this->disconnect();
}

// ==============================
//  Public
// ==============================

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
        LOG_DEBUG("Connection string: " << conn_str);
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
        RAISE_ERROR("Postgres connection is not open.");
    }

    try {
        pqxx::work transaction(*this->conn);
        pqxx::result result = transaction.exec(query);
        transaction.commit();
        return result;
    } catch (const exception& e) {
        RAISE_ERROR("Error during query execution: " + string(e.what()));
    }
    return pqxx::result{};
}

void PostgresDatabaseConnection::begin_cursor(const string& cursor_name, const string& query) {
    if (!this->conn || !this->conn->is_open()) {
        RAISE_ERROR("Postgres connection is not open.");
    }
    if (this->transaction) {
        RAISE_ERROR("A transaction is already active. Close the current cursor first.");
    }
    this->transaction = make_unique<pqxx::work>(*this->conn);

    try {
        this->transaction->exec("DECLARE " + cursor_name + " CURSOR FOR " + query);
    } catch (const exception& e) {
        this->transaction.reset();
        RAISE_ERROR("Error executing cursor query: " + string(e.what()));
    }
}

pqxx::result PostgresDatabaseConnection::fetch_cursor(const string& cursor_name, size_t limit) {
    if (!this->transaction) {
        RAISE_ERROR("No active transaction. Call begin_cursor first.");
    }
    return this->transaction->exec("FETCH " + std::to_string(limit) + " FROM " + cursor_name);
}

void PostgresDatabaseConnection::close_cursor() {
    if (this->transaction) {
        try {
            this->transaction->commit();
        } catch (...) {
            // Ignore errors during commit on close
        }
        this->transaction.reset();
    }
}

/**
 * PostgresWrapper implementation
 */

static const unordered_set<pqxx::oid> TIME_TYPE_OIDS = {
    1082,  // date
    1083,  // time without time zone
    1114,  // timestamp without time zone
    1184,  // timestamp with time zone
    1186,  // interval
    1266,  // time with time zone
};

static bool is_time_type(pqxx::oid oid) { return TIME_TYPE_OIDS.count(oid) > 0; }

// ==============================
//  Construction / destruction
// ==============================

PostgresWrapper::PostgresWrapper(PostgresDatabaseConnection& db_conn,
                                 shared_ptr<BoundedSharedQueue> output_queue,
                                 MAPPER_TYPE mapper_type)
    : SQLWrapper(db_conn, mapper_type), db_conn(db_conn), output_queue(output_queue) {}

PostgresWrapper::~PostgresWrapper() {}

// ==============================
//  Public
// ==============================

Table PostgresWrapper::get_table(const string& schema_name, const string& table_name) {
    string query = R"(
        WITH table_info AS (
            SELECT
                pt.schemaname || '.' || pt.tablename AS full_table_name,
                pc.reltuples::bigint AS row_count
            FROM pg_tables pt
            JOIN pg_class pc ON pc.relname = pt.tablename
            JOIN pg_namespace pn ON pn.oid = pc.relnamespace
                                AND pn.nspname = pt.schemaname
            WHERE pt.schemaname = ')" +
                   schema_name + R"('
              AND pt.tablename  = ')" +
                   table_name + R"('
        ),
        column_info AS (
            SELECT
                table_schema || '.' || table_name AS full_table_name,
                string_agg(column_name, ',' ORDER BY ordinal_position) AS columns
            FROM information_schema.columns
            WHERE table_schema = ')" +
                   schema_name + R"('
              AND table_name   = ')" +
                   table_name + R"('
            GROUP BY table_schema, table_name
        ),
        pk_info AS (
            SELECT
                n.nspname || '.' || c.relname AS full_table_name,
                string_agg(a.attname, ',' ORDER BY a.attnum) AS pk_column
            FROM pg_class c
            JOIN pg_namespace n ON n.oid = c.relnamespace
            JOIN pg_index i ON i.indrelid = c.oid AND i.indisprimary
            JOIN pg_attribute a ON a.attrelid = c.oid AND a.attnum = ANY(i.indkey)
            WHERE n.nspname = ')" +
                   schema_name + R"('
              AND c.relname = ')" +
                   table_name + R"('
            GROUP BY n.nspname, c.relname
        ),
        fk_info AS (
            SELECT
                n.nspname || '.' || c.relname AS full_table_name,
                string_agg(
                    a.attname || '|' || fn.nspname || '.' || fc.relname,
                    ','
                ) AS fk_columns
            FROM pg_constraint con
            JOIN pg_class c ON c.oid = con.conrelid
            JOIN pg_namespace n ON n.oid = c.relnamespace
            JOIN pg_attribute a ON a.attrelid = c.oid AND a.attnum = ANY(con.conkey)
            JOIN pg_class fc ON fc.oid = con.confrelid
            JOIN pg_namespace fn ON fn.oid = fc.relnamespace
            WHERE con.contype = 'f'
              AND n.nspname = ')" +
                   schema_name + R"('
              AND c.relname = ')" +
                   table_name + R"('
            GROUP BY n.nspname, c.relname
        )
        SELECT
            ti.full_table_name          AS table_name,
            COALESCE(ti.row_count, 0)   AS row_count,
            COALESCE(ci.columns,   '')  AS columns,
            COALESCE(pk.pk_column, '')  AS pk_column,
            COALESCE(fk.fk_columns,'')  AS fk_columns
        FROM      table_info  ti
        LEFT JOIN column_info ci ON ci.full_table_name = ti.full_table_name
        LEFT JOIN pk_info     pk ON pk.full_table_name = ti.full_table_name
        LEFT JOIN fk_info     fk ON fk.full_table_name = ti.full_table_name;
    )";

    auto result = db_conn.execute_query(query);

    if (result.empty()) {
        RAISE_ERROR("Table '" + schema_name + "." + table_name + "' not found.");
    }

    const auto& row = result[0];

    string cols = row["columns"].c_str();
    string pk_col = row["pk_column"].c_str();
    string fk_cols = row["fk_columns"].c_str();

    Table t;
    t.name = row["table_name"].c_str();
    t.row_count = row["row_count"].as<int>(0);
    t.primary_key = pk_col;
    t.column_names = cols.empty() ? vector<string>{} : Utils::split(cols, ',');
    t.foreign_keys = fk_cols.empty() ? vector<string>{} : Utils::split(fk_cols, ',');

    return t;
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
                RAISE_ERROR("Invalid foreign key format: " + fk);
            }

            string column = parts[0];
            string ref_table_name = parts[1];

            // Collect distinct non-null foreign-key values
            auto fk_ids = this->collect_fk_ids(table.name, column, where_clauses);

            if (fk_ids.empty()) continue;
            auto table_name_parts = Utils::split(ref_table_name, '.');
            auto ref_table = this->get_table(table_name_parts[0], table_name_parts[1]);
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

void PostgresWrapper::map_sql_query(const string& virtual_name, const string& raw_query) {
    map<string, vector<string>> table_columns_map = this->extract_aliases_from_query(raw_query);

    if (table_columns_map.empty()) {
        RAISE_ERROR("No valid aliases found in query for " + virtual_name);
    }

    map<string, Table> tables_metadata;

    // Search metadata (PK, FK, ...) of each referenced table
    // and validate that each table has its PK included in the aliases
    for (const auto& table_columns : table_columns_map) {
        string table_name = table_columns.first;
        vector<string> columns = table_columns.second;
        try {
            auto table_name_parts = Utils::split(table_name, '.');
            tables_metadata[table_name] = this->get_table(table_name_parts[0], table_name_parts[1]);
            string pk = tables_metadata[table_name].primary_key;
            if (find(columns.begin(), columns.end(), pk) == columns.end()) {
                auto parts = Utils::split(table_name, '.');
                string schema = parts[0];
                string table = parts[1];
                RAISE_ERROR("Primary key '" + pk + "' of table '" + table_name +
                            "' must be included in SELECT aliases. Add: " + table + "." + pk + " AS " +
                            schema + "_" + table + "__" + pk);
            }
        } catch (const exception& e) {
            RAISE_ERROR("Error retrieving metadata for table '" + table_name + "': " + e.what());
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

// ==============================
//  Private
// ==============================

vector<string> PostgresWrapper::build_columns_to_map(const Table& table,
                                                     const vector<string>& skip_columns) {
    for (const auto& skipo_col : skip_columns) {
        if (find(table.column_names.begin(), table.column_names.end(), skipo_col) ==
            table.column_names.end()) {
            RAISE_ERROR("Skip column '" + skipo_col + "' not found in table '" + table.name + "'");
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
    LOG_INFO("[START] Mapping table " << table.name);

    size_t limit = 100000;
    int rows_count = 0;
    int atoms_count = 0;

    string table_name = table.name;
    Utils::replace_all(table_name, ".", "_");
    string cursor_name = "cursor_" + table_name;

    this->db_conn.begin_cursor(cursor_name, query);

    while (true) {
        pqxx::result rows = this->db_conn.fetch_cursor(cursor_name, limit);

        LOG_DEBUG("Fetched " << rows.size() << " rows from table " << table.name);

        if (rows.empty()) break;

        while (this->get_available_ram_ratio() < 0.1) {
            LOG_INFO("Low available RAM. Waiting before adding more atoms to the queue...");
            Utils::sleep(5000);
        }

        for (const auto& row : rows) {
            SqlRow sql_row = this->build_sql_row(row, table, columns);

            LOG_DEBUG("Built SqlRow for table "
                      << table.name << " with primary key: "
                      << (sql_row.primary_key ? sql_row.primary_key->value : "NULL"));
#if LOG_LEVEL >= DEBUG_LEVEL
            for (const auto& field : sql_row.fields) {
                LOG_DEBUG("  Field: " << field.name << " = " << field.value);
            }
#endif

            vector<Atom*> atoms = this->mapper->map(DbInput{sql_row});

            if (this->mapper_type == MAPPER_TYPE::SQL2ATOMS) {
                atoms_count += atoms.size();

                std::queue<Atom*>* batch_queue = new std::queue<Atom*>();
                for (const auto& atom : atoms) {
                    batch_queue->push(atom);
                }
                unique_lock<mutex> lock(this->api_mutex);
                this->output_queue->enqueue((void*) batch_queue);
            } else {
                RAISE_ERROR("Unsupported mapper type.");
            }

            LOG_DEBUG("Mapper HandleTrie size: " << this->mapper->handle_trie_size());

            rows_count++;

            this->log_progress(table.name, rows_count);
        }
        LOG_INFO("Mapping table " << table.name << ". Total atoms generated: " << atoms_count);
    }

    this->db_conn.close_cursor();

    LOG_INFO("[END] Mapping table " << table.name << ". Total atoms generated: " << atoms_count);
    LOG_DEBUG("[END] Mapper HandleTrie size: " << this->mapper->handle_trie_size());
}

SqlRow PostgresWrapper::build_sql_row(const pqxx::row& row, const Table& table, vector<string> columns) {
    SqlRow sql_row;
    sql_row.table_name = table.name;
    sql_row.primary_key = ColumnValue{columns[0], row[0].c_str()};

    for (size_t i = 1; i < columns.size() && i < row.size(); i++) {
        auto field = row[i];

        if (field.is_null()) continue;

        if (is_time_type(field.type())) continue;

        string value = field.c_str();

        if (value.empty() || value.size() > MAX_VALUE_SIZE) {
            continue;
        }

        Utils::replace_all(value, "\n", " ");
        Utils::replace_all(value, "\t", " ");
        value = Utils::trim(value);

        string column_name = columns[i];
        for (const auto& fk : table.foreign_keys) {
            if (fk.find(columns[i]) != string::npos) {
                column_name = fk;
                break;
            }
        }
        sql_row.add_field(column_name, value);
    }
    return sql_row;
}

void PostgresWrapper::log_progress(const string& table_name, int rows_count) {
    int last_logged_count = this->tables_rows_count[table_name];

    if (rows_count - last_logged_count >= 50000) {
        LOG_INFO("Mapped " << rows_count << " rows from the " << table_name << " table");
        this->tables_rows_count[table_name] = rows_count;
    }
}

double PostgresWrapper::get_available_ram_ratio() {
    unsigned long total = Utils::get_total_ram();
    if (total == 0) return 0.0;
    return static_cast<double>(Utils::get_current_free_ram()) / static_cast<double>(total);
}
