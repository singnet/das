#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Atom.h"

using namespace std;
using namespace atoms;

namespace db_adapter {

namespace db_adapter_types {

// using DbScalar = variant<nullptr_t, bool, int64_t, double, string>;

struct ColumnValue {
    string name;
    string value;
};

struct SqlRow {
    string table_name;
    optional<ColumnValue> primary_key;
    vector<ColumnValue> fields;

    void add_field(string name, string value) { fields.push_back(ColumnValue{move(name), move(value)}); }

    optional<string> get(string name) const {
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

struct NoSqlDocument {};

using DbInput = variant<SqlRow, NoSqlDocument>;

using OutputList = variant<vector<string>, vector<Atom*>>;

struct Table {
    string name;
    int row_count = 0;
    vector<string> column_names;
    string primary_key;
    vector<string> foreign_keys;
};

enum MAPPER_TYPE { SQL2METTA, SQL2ATOMS };

}  // namespace db_adapter_types

}  // namespace db_adapter