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

    json contexts = json::parse(f);

    for (size_t i = 0; i < contexts.size(); ++i) {
        string msg_base = "contexts[" + to_string(i) + "]";

        const json& ctx = contexts[i];

        if (!ctx.contains("type")) {
            LOG_ERROR(msg_base + " missing required key: 'type'");
            has_error = true;
            continue;
        }

        if (!ctx["type"].is_number_integer()) {
            LOG_ERROR(msg_base + " 'type' must be integer 1 or 2");
            has_error = true;
            continue;
        }

        int type = ctx["type"].get<int>();

        if (type != 1 && type != 2) {
            LOG_ERROR(msg_base + " 'type' must be either 1 or 2");
            has_error = true;
            continue;
        }

        if (type == 1) {
            if (!ctx.contains("tables")) {
                LOG_ERROR(msg_base + " (type 1) missing required key: 'tables'");
                has_error = true;
                continue;
            }
            if (!ctx["tables"].is_array()) {
                LOG_ERROR(msg_base + ".tables must be an array in context of type 1");
                has_error = true;
                continue;
            }

            const json& tables = ctx["tables"];

            TableMapping tm;

            for (size_t t = 0; t < tables.size(); ++t) {
                string msg_tbase = msg_base + ".tables[" + to_string(t) + "]";

                const json& table = tables[t];

                if (!table.contains("table_name")) {
                    LOG_ERROR(msg_tbase + " missing required key: 'table_name'");
                    has_error = true;
                } else if (!table["table_name"].is_string()) {
                    LOG_ERROR(msg_tbase + ".table_name must be a string in a table entry");
                    has_error = true;
                } else {
                    string tn = table["table_name"].get<string>();
                    size_t count_dot = 0;
                    for (char c : tn)
                        if (c == '.') ++count_dot;
                    if (count_dot != 1) {
                        LOG_ERROR(msg_tbase +
                                  ".table_name must be in format 'schema.table' (single dot)");
                        has_error = true;
                    } else {
                        size_t pos = tn.find('.');
                        if (pos == 0 || pos + 1 >= tn.size()) {
                            LOG_ERROR(msg_tbase +
                                      ".table_name parts must not be empty in a table entry");
                            has_error = true;
                        }
                    }
                }

                tm.table_name = table["table_name"];

                if (!table.contains("skip_columns")) {
                    LOG_ERROR(msg_tbase + " missing required key: 'skip_columns'");
                    has_error = true;
                } else {
                    const json& sc = table["skip_columns"];
                    if (!sc.is_null()) {
                        if (!sc.is_array()) {
                            LOG_ERROR(
                                msg_tbase +
                                ".skip_columns must be an array of strings or null in a table entry");
                            has_error = true;
                        } else {
                            tm.skip_columns.emplace();
                            for (size_t k = 0; k < sc.size(); ++k) {
                                if (!sc[k].is_string()) {
                                    LOG_ERROR(msg_tbase + ".skip_columns[" + to_string(k) +
                                              "] must be a string in a table entry");
                                    has_error = true;
                                }
                                tm.skip_columns->push_back(sc[k]);
                            }
                        }
                    }
                }

                if (!table.contains("where_clauses")) {
                    LOG_ERROR(msg_tbase + " missing required key: 'where_clauses'");
                    has_error = true;
                } else {
                    const json& wc = table["where_clauses"];
                    if (!wc.is_null()) {
                        if (!wc.is_array()) {
                            LOG_ERROR(
                                msg_tbase +
                                ".where_clauses must be an array of strings or null in a table entry");
                            has_error = true;
                        } else {
                            tm.where_clauses.emplace();
                            for (size_t k = 0; k < wc.size(); ++k) {
                                if (!wc[k].is_string()) {
                                    LOG_ERROR(msg_tbase + ".where_clauses[" + to_string(k) +
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
        } else if (type == 2) {
            if (!ctx.contains("queries")) {
                LOG_ERROR(msg_base + " (type 2) missing required key: 'queries'");
                has_error = true;
                continue;
            }

            if (!ctx["queries"].is_array()) {
                LOG_ERROR(msg_base + ".queries must be an array");
                has_error = true;
                continue;
            }

            const json& queries = ctx["queries"];

            TableMapping tmq;

            for (size_t q = 0; q < queries.size(); ++q) {
                string msg_qbase = msg_base + ".queries[" + to_string(q) + "]";

                const json& query = queries[q];

                if (!query.contains("virtual_name")) {
                    LOG_ERROR(msg_qbase + " missing required key: 'virtual_name'.");
                    has_error = true;
                } else if (!query["virtual_name"].is_string()) {
                    LOG_ERROR(msg_qbase + ".virtual_name must be a string.");
                    has_error = true;
                }

                tmq.table_name = query["virtual_name"];

                if (!query.contains("query")) {
                    LOG_ERROR(msg_qbase + " missing required key: 'query'.");
                    has_error = true;
                } else if (!query["query"].is_string()) {
                    LOG_ERROR(msg_qbase + ".query must be a string.");
                    has_error = true;
                }

                tmq.query = query["query"];

                out.push_back(tmq);
            }
        } else {
            LOG_INFO("Type unknown");
        }
    }

    if (has_error) {
        LOG_ERROR("Context file validation failed with errors. Please fix the issues and try again.");
        return false;
    }
    return true;
}