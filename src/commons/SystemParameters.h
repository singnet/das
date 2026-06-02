#pragma once

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "JsonConfig.h"
#include "Properties.h"

using namespace std;

namespace commons {

class SystemParameters {
   public:
    SystemParameters(const nlohmann::json& params);
    SystemParameters(const JsonConfig& das_config);

    // Agents parameters getters
    Properties get_base_query_params() const;
    Properties get_query_agent_params() const;
    Properties get_link_creation_agent_params() const;
    Properties get_inference_agent_params() const;
    Properties get_evolution_agent_params() const;
    Properties get_context_agent_params() const;

   private:
    Properties get_agent_params(const string& agent) const;
    map<string, Properties> params_by_agent;
};

}  // namespace commons
