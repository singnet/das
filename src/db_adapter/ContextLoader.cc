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
        bool is_comment = line.size() >= 2 && line[0] == '-' && line[1] == '-';
        if (!line.empty() && !is_comment) {
            query += line + " ";
        } else {
            queries.push_back(query);
            query.clear();
        }
    }

    if (!query.empty()) {
        queries.push_back(query);
    }

    return queries;
}

vector<string> ContextLoader::load_metta_queries(const string& file_path) {
    RAISE_ERROR("ContextLoader::load_metta_queries() not implemented yet");
    return {};
}
