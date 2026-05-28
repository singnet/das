#include "SystemParameters.h"

#include "Utils.h"

using namespace commons;
using nlohmann::json;
using namespace std;

namespace {

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

void load_agent_params(const json& agent_params,
                       const string& agent,
                       map<string, Properties>& params_by_agent) {
    if (!agent_params.is_object()) {
        RAISE_ERROR("Parameters for agent '" + agent + "' must be an object");
    }
    Properties agent_props;
    for (auto pit = agent_params.begin(); pit != agent_params.end(); ++pit) {
        const string key = pit.key();
        agent_props[key] = json_scalar_to_property(pit.value(), agent + ".params." + key);
    }
    params_by_agent[agent] = agent_props;
}

void load_from_agents(const json& agents, map<string, Properties>& params_by_agent) {
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
        load_agent_params(*agent_params_it, agent + ".params", params_by_agent);
    }
}

}  // namespace

SystemParameters::SystemParameters(const json& root) {
    if (!root.is_object() || !root.contains("agents")) {
        RAISE_ERROR("Missing 'agents' section in parameters JSON");
    }
    load_from_agents(root["agents"], this->params_by_agent);
}

SystemParameters::SystemParameters(const JsonConfig& das_config) {
    auto agents = das_config.at_path("agents");
    if (agents.is_null()) {
        RAISE_ERROR("Missing 'agents' section in system config");
    }
    load_from_agents(*agents, this->params_by_agent);
}

Properties SystemParameters::get_agent_params(const string& agent) const {
    auto agent_it = this->params_by_agent.find(agent + ".params");
    if (agent_it == this->params_by_agent.end()) {
        RAISE_ERROR("Unknown agent: '" + agent + "'");
    }
    return agent_it->second;
}

Properties SystemParameters::get_base_query_params() const { return get_agent_params("base_query"); }

Properties SystemParameters::get_query_agent_params() const {
    return get_base_query_params() + get_agent_params("query");
}

Properties SystemParameters::get_link_creation_agent_params() const {
    return get_agent_params("link_creation");
}

Properties SystemParameters::get_inference_agent_params() const {
    return get_base_query_params() + get_agent_params("inference");
}

Properties SystemParameters::get_evolution_agent_params() const {
    return get_base_query_params() + get_agent_params("evolution");
}

Properties SystemParameters::get_context_agent_params() const {
    return get_base_query_params() + get_agent_params("context");
}
