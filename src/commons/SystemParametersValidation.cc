#include "SystemParametersValidation.h"

#include <unordered_map>

#include "Utils.h"

using namespace commons;
using nlohmann::json;
using namespace std;

namespace {

using FieldTypeSchema = unordered_map<string, string>;
using AgentParamsSchema = unordered_map<string, FieldTypeSchema>;

const AgentParamsSchema& schema_v1_0() {
    static const AgentParamsSchema schema = {
        {"base_query",
         {{"unique_assignment_flag", "bool"},
          {"attention_update", "long"},
          {"attention_correlation", "long"},
          {"max_bundle_size", "unsigned_int"},
          {"max_answers", "unsigned_int"},
          {"use_link_template_cache", "bool"},
          {"populate_metta_mapping", "bool"},
          {"use_metta_as_query_tokens", "bool"},
          {"allow_incomplete_chain_path", "bool"}}},
        {"query",
         {{"positive_importance_flag", "bool"},
          {"disregard_importance_flag", "bool"},
          {"unique_value_flag", "bool"},
          {"count_flag", "bool"}}},
        {"link_creation",
         {{"max_answers", "unsigned_int"},
          {"repeat_count", "unsigned_int"},
          {"context", "string"},
          {"attention_update", "long"},
          {"attention_correlation", "long"},
          {"positive_importance_flag", "bool"},
          {"query_interval", "long"},
          {"query_timeout", "long"},
          {"use_metta_as_query_tokens", "bool"}}},
        {"inference",
         {{"inference_request_timeout", "long"},
          {"repeat_count", "unsigned_int"},
          {"max_answers", "unsigned_int"}}},
        {"evolution",
         {{"population_size", "unsigned_int"},
          {"max_generations", "unsigned_int"},
          {"elitism_rate", "double"},
          {"selection_rate", "double"}}},
        {"context",
         {{"context", "string"},
          {"use_cache", "bool"},
          {"enforce_cache_recreation", "bool"},
          {"initial_rent_rate", "double"},
          {"initial_spreading_rate_lowerbound", "double"},
          {"initial_spreading_rate_upperbound", "double"}}},
    };
    return schema;
}

const unordered_map<string, const AgentParamsSchema*>& schemas_by_version() {
    static const AgentParamsSchema v1 = schema_v1_0();
    static const unordered_map<string, const AgentParamsSchema*> registry = {
        {SystemParametersValidation::SCHEMA_VERSION_1_0, &v1},
    };
    return registry;
}

const AgentParamsSchema* agent_schema_for_version(const string& schema_version) {
    auto registry_it = schemas_by_version().find(schema_version);
    if (registry_it == schemas_by_version().end()) {
        RAISE_ERROR("Unsupported system parameters schema version: '" + schema_version + "'");
    }
    return registry_it->second;
}

PropertyValue json_scalar_to_property(const json& value, const string& key) {
    if (value.is_string()) return value.get<string>();
    if (value.is_number_unsigned()) return value.get<unsigned int>();
    if (value.is_number_integer()) return static_cast<long>(value.get<long>());
    if (value.is_number_float()) return value.get<double>();
    if (value.is_boolean()) return value.get<bool>();
    if (value.is_null()) return string("");
    RAISE_ERROR("Invalid non-scalar parameter value for key '" + key + "'");
    return string("");
}

bool property_matches_type(const PropertyValue& value, const string& expected_type) {
    if (expected_type == "string") {
        return holds_alternative<string>(value);
    }
    if (expected_type == "bool") {
        return holds_alternative<bool>(value);
    }
    if (expected_type == "double") {
        return holds_alternative<double>(value);
    }
    if (expected_type == "long") {
        return holds_alternative<long>(value) || holds_alternative<unsigned int>(value);
    }
    if (expected_type == "unsigned_int") {
        return holds_alternative<unsigned int>(value) || holds_alternative<long>(value);
    }
    return false;
}

Properties parse_agent_params_with_schema(const json& agent_params,
                                          const string& agent,
                                          const AgentParamsSchema& agent_schema) {
    if (!agent_params.is_object()) {
        RAISE_ERROR("Parameters for agent '" + agent + "' must be an object");
    }

    const auto schema_it = agent_schema.find(agent);
    const bool has_schema = schema_it != agent_schema.end();

    if (has_schema) {
        for (const auto& field : schema_it->second) {
            if (!agent_params.contains(field.first)) {
                RAISE_ERROR("Missing required parameter '" + field.first + "' for agent '" + agent +
                            "'");
            }
        }
    }

    Properties agent_props;
    for (auto pit = agent_params.begin(); pit != agent_params.end(); ++pit) {
        const string key = pit.key();
        const string path = agent + ".params." + key;

        if (has_schema) {
            auto field_it = schema_it->second.find(key);
            if (field_it == schema_it->second.end()) {
                RAISE_ERROR("Unknown parameter '" + key + "' for agent '" + agent + "'");
            }
            PropertyValue value = json_scalar_to_property(pit.value(), path);
            if (!property_matches_type(value, field_it->second)) {
                RAISE_ERROR("Invalid type for parameter '" + key + "' for agent '" + agent +
                            "' (expected " + field_it->second + ")");
            }
            agent_props[key] = value;
        } else {
            agent_props[key] = json_scalar_to_property(pit.value(), path);
        }
    }
    return agent_props;
}

}  // namespace

void SystemParametersValidation::validate_schema_version(const string& schema_version) {
    agent_schema_for_version(schema_version);
}

bool SystemParametersValidation::is_supported_schema_version(const string& schema_version) {
    return schemas_by_version().find(schema_version) != schemas_by_version().end();
}

Properties SystemParametersValidation::parse_agent_params(const json& agent_params,
                                                          const string& agent,
                                                          const string& schema_version) {
    return parse_agent_params_with_schema(
        agent_params, agent, *agent_schema_for_version(schema_version));
}

void SystemParametersValidation::load_from_agents(const json& agents,
                                                  const string& schema_version,
                                                  map<string, Properties>& params_by_agent) {
    validate_schema_version(schema_version);
    if (!agents.is_object()) {
        RAISE_ERROR("'agents' must be a JSON object");
    }
    for (auto it = agents.begin(); it != agents.end(); ++it) {
        const string agent = it.key();
        const json& agent_json = it.value();
        if (!agent_json.is_object()) {
            continue;
        }
        auto agent_params_it = agent_json.find("params");
        if (agent_params_it == agent_json.end() || agent_params_it->is_null()) {
            continue;
        }
        params_by_agent[agent] = parse_agent_params(*agent_params_it, agent, schema_version);
    }
}
