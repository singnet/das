#include "ContextLoader.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace commons;

using json = nlohmann::json;

namespace fs = std::filesystem;

vector<string> ContextLoader::load_sql_queries(const string& file_path) {
    if (!fs::exists(file_path)) {
        RAISE_ERROR("Query file " + file_path + " does not exist");
    }

    ifstream f(file_path);

    vector<string> queries;
    string query;
    string line;

    while (getline(f, line)) {
        line = Utils::trim(line);
        bool is_comment = Utils::starts_with(line, "--");
        if (!line.empty() && !is_comment) {
            query += line + " ";
        } else {
            if (!query.empty()) {
                queries.push_back(query);
                query.clear();
            }
        }
    }

    if (!query.empty()) {
        queries.push_back(query);
    }

    return queries;
}

vector<string> ContextLoader::load_metta_queries(const string& file_path) {
    if (!fs::exists(file_path)) {
        RAISE_ERROR("File " + file_path + " does not exist");
    }

    ifstream file(file_path);

    vector<string> expressions;
    string line;
    string current_expression;
    int parentheses_depth = 0;

    while (getline(file, line)) {
        line = Utils::trim(line);
        bool is_comment = Utils::starts_with(line, ";");

        if (line.empty() || is_comment) continue;

        LOG_DEBUG("Read line: " + line);

        if (!current_expression.empty()) {
            current_expression += " ";
        }
        current_expression += line;

        for (char c : line) {
            if (c == '(')
                parentheses_depth++;
            else if (c == ')')
                parentheses_depth--;
        }

        if (parentheses_depth == 0 && !current_expression.empty()) {
            expressions.push_back(current_expression);
            current_expression.clear();
        }
    }

    if (parentheses_depth != 0) {
        RAISE_ERROR("Error in queries " + file_path);
    }

    return expressions;
}