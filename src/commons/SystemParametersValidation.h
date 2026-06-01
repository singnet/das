#pragma once

#include <map>
#include <nlohmann/json.hpp>
#include <string>

#include "Properties.h"

using namespace std;

namespace commons {

/**
 * Validates and parses agent params from das.json against a versioned field schema.
 * Schema version is read from agents.schema_version in das.json (e.g. "1.0").
 */
class SystemParametersValidation {
   public:
    static string SCHEMA_VERSION;
    static string params_schema_to_json_string();
    static void load_from_agents(const nlohmann::json& agents, map<string, Properties>& params_by_agent);
};

}  // namespace commons
