#include "SystemParameters.h"

#include "JsonConfig.h"
#include "SystemParametersValidation.h"
#include "Utils.h"

using namespace commons;
using nlohmann::json;
using namespace std;

SystemParameters::SystemParameters(const json& root) {
    if (!root.is_object() || !root.contains("agents")) {
        RAISE_ERROR("Missing 'agents' section in parameters JSON");
    }
    SystemParametersValidation::load_from_agents(root["agents"], this->params_by_agent);
}

SystemParameters::SystemParameters(const JsonConfig& das_config) {
    auto agents = das_config.at_path("agents");
    if (agents.is_null()) {
        RAISE_ERROR("Missing 'agents' section in system config");
    }
    SystemParametersValidation::load_from_agents(*agents, this->params_by_agent);
}

Properties SystemParameters::get_agent_params(const string& agent) const {
    auto agent_it = this->params_by_agent.find(agent);
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

Properties SystemParameters::get_command_router_params() const {
    return get_base_query_params() + get_agent_params("query") + get_agent_params("evolution") +
           get_agent_params("context");
}
