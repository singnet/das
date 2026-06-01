#include "JsonConfigParser.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "Utils.h"

using namespace std;
using nlohmann::json;

namespace commons {

namespace {

const vector<string> required_fields_by_version() {
    static const vector<string> required = {"atomdb", "atomdb.type", "loaders", "agents"};
    return required;
}

JsonConfig parse_and_validate(const string& json_str) {
    JsonConfig config;
    try {
        config = JsonConfig(json::parse(json_str));
    } catch (const exception& e) {
        RAISE_ERROR("Invalid JSON in config: " + string(e.what()));
    }
    for (const string& key : required_fields_by_version()) {
        if (config.at_path(key).is_null()) {
            RAISE_ERROR("Missing required field: " + key);
        }
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
    return parse_and_validate(buf.str());
}

JsonConfig JsonConfigParser::load_from_string(const string& json_content) {
    return parse_and_validate(json_content);
}

}  // namespace commons
