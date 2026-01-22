#pragma once

#include <string>
#include <vector>
#include <variant>
#include <unordered_map>

using namespace std;

struct MappedData;
struct SQLSourceData;
struct NoSQLSourceData;
struct Table;

namespace db_adapter {

namespace db_adapter_types {

/**
 * Representing the mapped data after processing.
 */
struct MappedData {
    vector<variant<int, double, string>> mapped_atoms;
};

/**
 * Representing SQL-based source data. 
 */
struct SQLSourceData {
    string table_name;
    unordered_map<string, variant<int, double, string>> row_data;
};

/**
 * Representing NoSQL source data (empty structure).
 */
struct NoSQLSourceData {};

// TypeAlias equivalente:  SourceData could be SQL or NoSQL
using SourceData = variant<SQLSourceData, NoSQLSourceData>;

struct Table {
    std::string name;
    int row_count = 0;
    std::vector<std::string> column_names;
    std::string primary_key;
    std::vector<std::string> foreign_keys;
};

} // namespace db_adapter_types

} // namespace db_adapter