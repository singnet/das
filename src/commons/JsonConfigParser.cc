#include "JsonConfigParser.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "Utils.h"

using namespace std;
using nlohmann::json;

namespace commons {

namespace {

// bus_node: required fields.
const unordered_map<string, vector<string>>& required_node_fields_by_version() {
    static const unordered_map<string, vector<string>> m = {
        {"1.0", {"atomdb", "atomdb.type", "loaders", "agents", "brokers"}},
    };
    return m;
}

// bus_client: required fields.
const unordered_map<string, vector<string>>& required_client_fields_by_version() {
    static const unordered_map<string, vector<string>> m = {
        {"1.0", {"params", "params.das-config-file", "params.bus-endpoint", "params.ports-range"}},
    };
    return m;
}

string required_fields_error(const string& version, const vector<string>& missing) {
    string msg = "Config schema version \"" + version +
                 "\" requires the following fields that are missing or null: ";
    for (size_t i = 0; i < missing.size(); ++i) {
        if (i > 0) msg += ", ";
        msg += "\"" + missing[i] + "\"";
    }
    msg += ". Please add them to your config file or use a compatible schema version.";
    return msg;
}

void validate_schema_version(const JsonConfig& config, vector<string>& required) {
    vector<string> missing;
    for (const string& key : required) {
        if (config.at_path(key).is_null()) {
            missing.push_back(key);
        }
    }
    if (!missing.empty()) {
        string version = config.get_schema_version();
        Utils::error(required_fields_error(version, missing), true);
    }
}

JsonConfig parse_and_validate(const string& json_str, bool is_node) {
    JsonConfig config;
    try {
        config = JsonConfig(json::parse(json_str));
    } catch (const exception& e) {
        Utils::error("Invalid JSON in config: " + string(e.what()), true);
    }
    string version = config.get_schema_version();
    try {
        vector<string> required = is_node ? required_node_fields_by_version().at(version)
                                          : required_client_fields_by_version().at(version);
        validate_schema_version(config, required);
    } catch (const exception& e) {
        Utils::error("Invalid schema version: " + version + " " + string(e.what()), true);
    }
    return config;
}

}  // namespace

JsonConfig JsonConfigParser::load(const string& file_path, bool throw_flag) {
    ifstream f(file_path);
    if (!f.good()) {
        Utils::error("JsonConfigParser: Cannot open config file: " + file_path, throw_flag);
        return JsonConfig();
    }
    stringstream buf;
    buf << f.rdbuf();
    return parse_and_validate(buf.str(), true);
}

JsonConfig JsonConfigParser::load_from_string(const string& json_content) {
    return parse_and_validate(json_content, true);
}

JsonConfig JsonConfigParser::load_client_config(const string& file_path, bool throw_flag) {
    ifstream f(file_path);
    if (!f.good()) {
        Utils::error("JsonConfigParser: Cannot open client config file: " + file_path, throw_flag);
        return JsonConfig();
    }
    stringstream buf;
    buf << f.rdbuf();
    return parse_and_validate(buf.str(), false);
}

JsonConfig JsonConfigParser::load_client_config_from_string(const string& json_content) {
    return parse_and_validate(json_content, false);
}

}  // namespace commons
