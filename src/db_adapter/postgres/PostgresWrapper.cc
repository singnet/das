#include "PostgresWrapper.h"

#include <iostream>
#include <memory>
#include <pqxx/pqxx>
#include <stdexcept>
#include <string>

#include "Logger.h"
#include "Node.h"

using namespace std;

PostgresWrapper::PostgresWrapper(const string& host,
                                 int port,
                                 const string& database,
                                 const string& user,
                                 const string& password,
                                 db_adapter_types::MAPPER_TYPE mapper_type)
    : SQLWrapper<pqxx::connection>(mapper_type),
      host(host),
      port(port),
      database(database),
      user(user),
      password(password) {
    this->db_client = this->connect();
}

PostgresWrapper::~PostgresWrapper() = default;

unique_ptr<pqxx::connection> PostgresWrapper::connect() {
    try {
        string conn_str = "host=" + host + " port=" + to_string(port) + " dbname=" + database;
        if (!user.empty()) {
            conn_str += " user=" + user;
        }
        if (!password.empty()) {
            conn_str += " password=" + password;
        }
        return make_unique<pqxx::connection>(conn_str);
    } catch (const pqxx::sql_error& e) {
        throw runtime_error("Could not connect to database: " + string(e.what()));
    } catch (const exception& e) {
        throw runtime_error("Could not connect to database: " + string(e.what()));
    }
}

void PostgresWrapper::disconnect() {
    if (this->db_client) {
        this->db_client->close();
    }
}

Table PostgresWrapper::get_table(const string& name) {
    auto tables = this->list_tables();
    for (const auto& table : tables) {
        if (table.name == name) return table;
    }
    Utils::error("Table '" + name + "' not found in the database.");
    return Table{};
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
    auto result = this->execute_query(query);
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
    string where_clauses;

    for (size_t i = 0; i < clauses.size(); ++i) {
        if (i > 0) where_clauses += " AND ";
        where_clauses += clauses[i];
    }

    auto columns = build_columns_to_map(table, skip_columns);

    this->process_paginated(table, columns, where_clauses);

    if (second_level) {
        for (const auto& fk : table.foreign_keys) {
            auto pair = Utils::split(fk, '|');

            if (pair.size() != 2) {
                Utils::error("Invalid foreign key format: " + fk);
            }

            string column = pair[0];
            string ref_table_name = pair[1];

            // Collect distinct non-null foreign-key values
            auto fk_ids = this->collect_fk_ids(table.name, column, where_clauses);

            if (fk_ids.empty()) continue;

            auto ref_table = this->get_table(ref_table_name);
            auto ref_columns = this->build_columns_to_map(ref_table);

            string where_clause = ref_table.primary_key + " IN " + "(" + Utils::join(fk_ids, ',') + ")";

            this->process_paginated(ref_table, ref_columns, where_clause);
        }
    }
}

vector<string> PostgresWrapper::build_columns_to_map(const Table& table,
                                                     const vector<string>& skip_columns) {
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

void PostgresWrapper::process_paginated(const Table& table,
                                        const vector<string>& columns,
                                        const string& where_clauses) {
    size_t offset = 0;
    const size_t limit = 10000;
    string cols_sql = Utils::join(columns, ',');

    string base_query = "SELECT " + cols_sql + " FROM " + table.name;

    if (!where_clauses.empty()) {
        base_query += " WHERE " + where_clauses;
    }

    while (true) {
        string query = base_query + " LIMIT " + to_string(limit) + " OFFSET " + to_string(offset);
        pqxx::result rows = this->execute_query(query);

        if (rows.empty()) break;

        for (const auto& row : rows) {
            auto atoms = this->map_row_2_atoms(row, table, columns);
            // WIP - send atom to SharedQueue
        }

        offset += limit;
    }
}

vector<Atom*> PostgresWrapper::map_row_2_atoms(const pqxx::row& row,
                                               const Table& table,
                                               vector<string> columns) {
    SqlRow sql_row = this->build_sql_row(row, table, columns);
    auto atoms = this->mapper->map(DbInput{sql_row});

    LOG_INFO("====>>>>>> Atoms count: " << get<vector<Atom*>>(atoms).size());

    return get<vector<Atom*>>(atoms);
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

vector<string> PostgresWrapper::collect_fk_ids(const string& table_name,
                                               const string& column_name,
                                               const string& where_clause) {
    size_t offset = 0;
    size_t limit = 10000;
    vector<string> ids;

    while (true) {
        string query = "SELECT " + column_name + " FROM " + table_name + " WHERE " + where_clause +
                       " LIMIT " + to_string(limit) + " OFFSET " + to_string(offset) + ";";
        pqxx::result rows = this->execute_query(query);

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

pqxx::result PostgresWrapper::execute_query(const string& query) {
    if (!this->db_client || !this->db_client->is_open()) {
        Utils::error("Database connection is not open.");
    }

    try {
        pqxx::work transaction(*this->db_client);
        pqxx::result result = transaction.exec(query);
        transaction.commit();
        return result;
    } catch (const pqxx::sql_error& e) {
        Utils::error("SQL error during query execution: " + string(e.what()));
    } catch (const exception& e) {
        Utils::error("Error during query execution: " + string(e.what()));
    }
    return pqxx::result{};
}