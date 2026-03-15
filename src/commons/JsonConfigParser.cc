#include "JsonConfigParser.h"

#include <fstream>
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

// Required top-level keys per schema version.
const unordered_map<string, vector<string>>& required_fields_by_version() {
    static const unordered_map<string, vector<string>> m = {
        {"1.0", {"schema_version", "atomdb", "atomdb.type", "loaders", "agents", "brokers", "params"}},
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

void validate_schema_version(const JsonConfig& config) {
    string version = config.get_schema_version();
    auto it = required_fields_by_version().find(version);
    if (it == required_fields_by_version().end()) {
        string supported;
        for (const auto& p : required_fields_by_version()) {
            if (!supported.empty()) supported += ", ";
            supported += "\"" + p.first + "\"";
        }
        Utils::error("Config schema version \"" + version +
                         "\" is not supported. Supported versions: " + supported + ".",
                     true);
    }
    const vector<string>& required = it->second;
    vector<string> missing;
    for (const string& key : required) {
        if (config.at_path(key).is_null()) {
            missing.push_back(key);
        }
    }
    if (!missing.empty()) {
        Utils::error(required_fields_error(version, missing), true);
    }
}

JsonConfig parse_and_validate(const string& json_str) {
    JsonConfig config;
    try {
        config = JsonConfig(json::parse(json_str));
    } catch (const exception& e) {
        Utils::error("Invalid JSON in config: " + string(e.what()), true);
    }
    validate_schema_version(config);
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
    return parse_and_validate(buf.str());
}

JsonConfig JsonConfigParser::load_from_string(const string& json_content) {
    return parse_and_validate(json_content);
}

}  // namespace commons
