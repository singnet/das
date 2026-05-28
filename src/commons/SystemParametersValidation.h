#pragma once

#include <map>
#include <nlohmann/json.hpp>
#include <string>

#include "Properties.h"

using namespace std;

namespace commons {

/**
 * Validates and parses agent params from das.json against a versioned field schema.
 * Schema versions align with the top-level config schema_version (e.g. "1.0").
 */
class SystemParametersValidation {
   public:
    static constexpr const char* SCHEMA_VERSION_1_0 = "1.0";

    static void validate_schema_version(const string& schema_version);
    static bool is_supported_schema_version(const string& schema_version);

    static Properties parse_agent_params(const nlohmann::json& agent_params,
                                         const string& agent,
                                         const string& schema_version);

    static void load_from_agents(const nlohmann::json& agents,
                                 const string& schema_version,
                                 map<string, Properties>& params_by_agent);
};

}  // namespace commons
