#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "Atom.h"

using namespace std;
using namespace atoms;

namespace db_adapter {

/**
 * @struct ColumnValue
 * @brief Represents a single cell in a database row.
 */
struct ColumnValue {
    string name;
    string value;
};

/**
 * @struct SqlRow
 * @brief Represents a single row from a SQL query result.
 */
struct SqlRow {
    string table_name;
    optional<ColumnValue> primary_key;
    vector<ColumnValue> fields;

    /**
     * @brief Adds a field to the row.
     * @param name Column name.
     * @param value Column value.
     */
    void add_field(string name, string value) { fields.push_back(ColumnValue{move(name), move(value)}); }

    /**
     * @brief Retrieves a value by column name.
     * @param name The column name to search for.
     * @return optional<string> The value if found, otherwise nullopt.
     */
    optional<string> get(const string& name) const {
        if (primary_key && primary_key->name == name) {
            return primary_key->value;
        }
        for (const auto& field : fields) {
            if (field.name == name) {
                return field.value;
            }
        }
        return nullopt;
    }

    size_t size() const { return (primary_key ? 1 : 0) + fields.size(); }
};

struct MettaExpression {
    string expression;
};

struct NoSqlDocument {};

/**
 * @typedef DbInput
 * @brief A variant representing raw input from the database, which can be a SQL row, a NoSQL document,
 * or a Metta expression.
 */
using DbInput = variant<SqlRow, NoSqlDocument, MettaExpression>;

/**
 * @struct Table
 * @brief Metadata describing a database table.
 */
struct Table {
    string name;
    int row_count = 0;
    vector<string> column_names;
    string primary_key;
    vector<string> foreign_keys;
};

/**
 * @enum MAPPER_TYPE
 * @brief Defines the strategy used to transform database rows.
 */
enum class MAPPER_TYPE { SQL2ATOMS, MORK2ATOMS };

struct TableMapping {
    string table_name;
    optional<vector<string>> where_clauses = nullopt;
    optional<vector<string>> skip_columns = nullopt;
};

}  // namespace db_adapter