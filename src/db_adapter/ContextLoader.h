#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "DataTypes.h"
#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace commons;
using namespace db_adapter;
using json = nlohmann::json;

namespace fs = std::filesystem;

bool load_context_file(const string& file_path, vector<TableMapping>& out) {
    out.clear();

    bool has_error = false;

    if (!fs::exists(file_path)) {
        LOG_ERROR("Context file " + file_path + " does not exist");
        has_error = true;
    }

    ifstream f(file_path);

    json tables = json::parse(f);

    for (size_t i = 0; i < tables.size(); ++i) {
        string msg_base = "table[" + to_string(i) + "]";

        const json& table = tables[i];

        TableMapping tm;

        if (!table.contains("table_name")) {
            LOG_ERROR(msg_base + " missing required key: 'table_name'");
            has_error = true;
        } else if (!table["table_name"].is_string()) {
            LOG_ERROR(msg_base + ".table_name must be a string in a table entry");
            has_error = true;
        } else {
            string tn = table["table_name"].get<string>();
            size_t count_dot = 0;
            for (char c : tn)
                if (c == '.') ++count_dot;
            if (count_dot != 1) {
                LOG_ERROR(msg_base + ".table_name must be in format 'schema.table' (single dot)");
                has_error = true;
            } else {
                size_t pos = tn.find('.');
                if (pos == 0 || pos + 1 >= tn.size()) {
                    LOG_ERROR(msg_base + ".table_name parts must not be empty in a table entry");
                    has_error = true;
                }
            }
        }

        tm.table_name = table["table_name"];

        if (!table.contains("skip_columns")) {
            LOG_ERROR(msg_base + " missing required key: 'skip_columns'");
            has_error = true;
        } else {
            const json& sc = table["skip_columns"];
            if (!sc.is_null()) {
                if (!sc.is_array()) {
                    LOG_ERROR(msg_base +
                              ".skip_columns must be an array of strings or null in a table entry");
                    has_error = true;
                } else {
                    tm.skip_columns.emplace();
                    for (size_t k = 0; k < sc.size(); ++k) {
                        if (!sc[k].is_string()) {
                            LOG_ERROR(msg_base + ".skip_columns[" + to_string(k) +
                                      "] must be a string in a table entry");
                            has_error = true;
                        }
                        tm.skip_columns->push_back(sc[k]);
                    }
                }
            }
        }

        if (!table.contains("where_clauses")) {
            LOG_ERROR(msg_base + " missing required key: 'where_clauses'");
            has_error = true;
        } else {
            const json& wc = table["where_clauses"];
            if (!wc.is_null()) {
                if (!wc.is_array()) {
                    LOG_ERROR(msg_base +
                              ".where_clauses must be an array of strings or null in a table entry");
                    has_error = true;
                } else {
                    tm.where_clauses.emplace();
                    for (size_t k = 0; k < wc.size(); ++k) {
                        if (!wc[k].is_string()) {
                            LOG_ERROR(msg_base + ".where_clauses[" + to_string(k) +
                                      "] must be a string in a table entry");
                            has_error = true;
                        }
                        tm.where_clauses->push_back(wc[k]);
                    }
                }
            }
        }
        
        out.push_back(tm);
    }

    if (has_error) {
        LOG_ERROR("Context file validation failed with errors. Please fix the issues and try again.");
        return false;
    }
    return true;
}