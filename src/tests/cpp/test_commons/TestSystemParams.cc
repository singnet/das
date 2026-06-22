#include "TestSystemParams.h"

#include <nlohmann/json.hpp>

using namespace commons;
using nlohmann::json;

namespace das_test {

const char kAgentsJson[] = R"({
  "agents": {
    "schema_version": "1.0.1",
    "base_query": {
      "params": {
        "unique_assignment_flag": false,
        "attention_update": 0,
        "attention_correlation": 0,
        "max_bundle_size": 1000,
        "max_answers": 0,
        "use_link_template_cache": false,
        "populate_metta_mapping": false,
        "use_metta_as_query_tokens": false,
        "allow_incomplete_chain_path": false
      }
    },
    "query": {
      "endpoint": "localhost:40002",
      "ports_range": "42000:42999",
      "params": {
        "positive_importance_flag": false,
        "disregard_importance_flag": false,
        "unique_value_flag": false,
        "count_flag": false,
        "attention_focus_strictness": 0.0
      }
    },
    "link_creation": {
      "params": {
        "max_answers": 10,
        "repeat_count": 1,
        "context": "context",
        "attention_update": 0,
        "attention_correlation": 0,
        "positive_importance_flag": true,
        "query_interval": 0,
        "query_timeout": 0,
        "use_metta_as_query_tokens": false
      }
    },
    "inference": {
      "params": {
        "inference_request_timeout": 86400,
        "repeat_count": 5,
        "max_answers": 150
      }
    },
    "evolution": {
      "params": {
        "population_size": 1000,
        "max_generations": 100,
        "elitism_rate": 0.01,
        "selection_rate": 0.1
      }
    },
    "context": {
      "params": {
        "context": "context",
        "use_cache": true,
        "enforce_cache_recreation": false,
        "initial_rent_rate": 0.75,
        "initial_spreading_rate_lowerbound": 0.1,
        "initial_spreading_rate_upperbound": 0.1
      }
    }
  }
})";

SystemParameters make_test_parameters() { return SystemParameters(json::parse(kAgentsJson)); }

void init_test_system_parameters_singleton() {
    SystemParametersSingleton::provide(std::make_shared<SystemParameters>(make_test_parameters()));
}

}  // namespace das_test
