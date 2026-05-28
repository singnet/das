#include <gtest/gtest.h>

#include "SystemParameters.h"
#include "SystemParametersSingleton.h"
#include "TestSystemParams.h"

using namespace commons;
using namespace das_test;
using namespace std;

TEST(SystemParametersTest, get_base_query_params) {
    auto params = make_test_parameters().get_base_query_params();
    EXPECT_EQ(params.get<unsigned int>("max_answers"), 0U);
    EXPECT_EQ(params.get<unsigned int>("max_bundle_size"), 1000U);
    EXPECT_FALSE(params.get<bool>("unique_assignment_flag"));
}

TEST(SystemParametersTest, get_query_agent_params_merges_base_and_query) {
    auto params = make_test_parameters().get_query_agent_params();
    EXPECT_FALSE(params.get<bool>("count_flag"));
    EXPECT_FALSE(params.get<bool>("unique_assignment_flag"));
    EXPECT_FALSE(params.get<bool>("positive_importance_flag"));
    EXPECT_EQ(params.get<unsigned int>("max_bundle_size"), 1000U);
    // Must be merged with base query params
    EXPECT_EQ(params.get<unsigned int>("max_answers"), 0U);
}

TEST(SystemParametersTest, get_link_creation_agent_params_standalone) {
    auto params = make_test_parameters().get_link_creation_agent_params();
    EXPECT_EQ(params.get<unsigned int>("max_answers"), 10U);
    EXPECT_TRUE(params.get<bool>("positive_importance_flag"));
    EXPECT_EQ(params.find("unique_assignment_flag"), params.end());
}

TEST(SystemParametersTest, get_inference_agent_params_overrides_base_max_answers) {
    auto params = make_test_parameters().get_inference_agent_params();
    EXPECT_EQ(params.get<unsigned int>("max_answers"), 150U);
    EXPECT_EQ(params.get<unsigned int>("repeat_count"), 5U);
    EXPECT_EQ(params.get<unsigned int>("inference_request_timeout"), 86400U);
}

TEST(SystemParametersTest, get_evolution_agent_params_merges_base) {
    auto params = make_test_parameters().get_evolution_agent_params();
    EXPECT_EQ(params.get<unsigned int>("population_size"), 1000U);
    EXPECT_EQ(params.get<unsigned int>("max_generations"), 100U);
    EXPECT_FALSE(params.get<bool>("unique_assignment_flag"));
}

TEST(SystemParametersTest, get_context_agent_params_merges_base) {
    auto params = make_test_parameters().get_context_agent_params();
    EXPECT_TRUE(params.get<bool>("use_cache"));
    EXPECT_DOUBLE_EQ(params.get<double>("initial_rent_rate"), 0.75);
    EXPECT_EQ(params.get<string>("context"), "context");
}

TEST(SystemParametersTest, singleton_matches_direct_instance) {
    init_test_system_parameters_singleton();
    auto from_singleton = SystemParametersSingleton::get_instance()->get_query_agent_params();
    auto direct = make_test_parameters().get_query_agent_params();
    EXPECT_EQ(from_singleton.get<unsigned int>("max_answers"), direct.get<unsigned int>("max_answers"));
    EXPECT_EQ(from_singleton.get<bool>("count_flag"), direct.get<bool>("count_flag"));
}

TEST(SystemParametersTest, properties_merge_prioritizes_right_hand_side) {
    Properties base;
    base["max_answers"] = 0U;
    base["count_flag"] = false;

    Properties agent;
    agent["max_answers"] = 100U;
    agent["count_flag"] = true;
    agent["unique_value_flag"] = true;

    Properties merged = base + agent;
    EXPECT_EQ(merged.get<unsigned int>("max_answers"), 100U);
    EXPECT_TRUE(merged.get<bool>("count_flag"));
    EXPECT_TRUE(merged.get<bool>("unique_value_flag"));
    EXPECT_EQ(merged.size(), 3U);
}
