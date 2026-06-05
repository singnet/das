#include "AtomDBSingleton.h"
#include "BusCommandRouterProcessor.h"
#include "BusCommandRouterProxy.h"
#include "EvolutionMettaParser.h"
#include "ServiceBus.h"
#include "TestAtomDBJsonConfig.h"
#include "TestSystemParams.h"
#include "Utils.h"
#include "gtest/gtest.h"

using namespace command_router;
using namespace service_bus;
using namespace atomdb;
using das_test::init_test_system_parameters_singleton;

class BusCommandRouterTestEnvironment : public ::testing::Environment {
   public:
    void SetUp() override {
        AtomDBSingleton::init(test_atomdb_json_config());
        init_test_system_parameters_singleton();
    }
    void TearDown() override {}
};

class CaptureForwardProxy : public BusCommandProxy {
   public:
    void pack_command_line_args() override {}
};

class CaptureForwardProcessor : public BusCommandProcessor {
   public:
    string last_requestor_id;
    string last_proxy_node_id;
    vector<string> last_packed_args;

    CaptureForwardProcessor() : BusCommandProcessor({ServiceBus::PATTERN_MATCHING_QUERY}) {}

    shared_ptr<BusCommandProxy> factory_empty_proxy() override {
        return make_shared<CaptureForwardProxy>();
    }

    void run_command(shared_ptr<BusCommandProxy> proxy) override {
        last_requestor_id = proxy->get_requestor_id();
        last_proxy_node_id = proxy->peer_id();
        last_packed_args = proxy->get_args();
    }
};

class RouterTestProxy : public BusCommandRouterProxy {
   public:
    RouterTestProxy(const string& router_command, const string& router_arg)
        : BusCommandRouterProxy(router_command, router_arg) {}

    bool from_remote_peer(const string& command, const vector<string>& args) override {
        if (command == BusCommandRouterProxy::PARAMS_RESPONSE && !args.empty()) {
            params_response = args[0];
            return true;
        }
        if (command == BusCommandRouterProxy::SET_PARAM_ACK && !args.empty()) {
            set_param_ack = args[0];
            return true;
        }
        if (command == BusCommandRouterProxy::ROUTED) {
            routed_flag = true;
            return true;
        }
        return BusCommandRouterProxy::from_remote_peer(command, args);
    }
};

TEST(EvolutionMettaParser, parse_labeled_evolution_arg_with_aliases) {
    EvolutionMettaArgs args;
    string metta_arg =
        "((query (Contains %sentence1 (Word \"bbb\"))) "
        "(ff count_letter) "
        "(cq ((Contains %placeholder1 %word1))) "
        "(cr (((placeholder1 sentence1)))) "
        "(cm (((sentence1 word1)))))";
    ASSERT_TRUE(try_parse_evolution_metta_arg(metta_arg, args));
    EXPECT_EQ(args.query, "(Contains %sentence1 (Word \"bbb\"))");
    EXPECT_EQ(args.fitness_function_tag, "count_letter");
    ASSERT_EQ(args.correlation_query_expressions.size(), 1u);
    EXPECT_EQ(args.correlation_query_expressions[0], "(Contains %placeholder1 %word1)");
    ASSERT_EQ(args.correlation_replacement_groups.size(), 1u);
    EXPECT_EQ(args.correlation_replacement_groups[0][0].first, "placeholder1");
    EXPECT_EQ(args.correlation_mapping_groups[0][0].second, "word1");
}

TEST(EvolutionMettaParser, reject_colon_syntax_in_cr_and_cm) {
    EvolutionMettaArgs args;
    string metta_arg =
        "((query (Contains %sentence1 (Word \"bbb\"))) "
        "(ff count_letter) "
        "(cq ((Contains %placeholder1 %word1))) "
        "(cr %placeholder1:sentence1) "
        "(cm sentence1:%word1))";
    EXPECT_THROW(try_parse_evolution_metta_arg(metta_arg, args), runtime_error);
}

TEST(EvolutionMettaParser, reject_two_level_cr_and_cm_layout) {
    EvolutionMettaArgs args;
    string metta_arg =
        "((query (Contains %sentence1 (Word \"bbb\"))) "
        "(ff count_letter) "
        "(cq ((Contains %placeholder1 %word1))) "
        "(cr ((placeholder1 sentence1))) "
        "(cm (((sentence1 word1)))))";
    EXPECT_THROW(try_parse_evolution_metta_arg(metta_arg, args), runtime_error);
}

TEST(EvolutionMettaParser, parse_simplified_wrapped_cr_cm_from_command_line) {
    EvolutionMettaArgs args;
    string metta_arg =
        "((query (Contains %sentence1 (Word \"bbb\"))) "
        "(ff count_letter) "
        "(cq ((Contains %placeholder1 %word1))) "
        "(cr (((placeholder1 sentence1)))) "
        "(cm (((sentence1 word1)))))";
    ASSERT_TRUE(try_parse_evolution_metta_arg(metta_arg, args));
    EXPECT_EQ(args.fitness_function_tag, "count_letter");
    ASSERT_EQ(args.correlation_query_expressions.size(), 1u);
    EXPECT_EQ(args.correlation_query_expressions[0], "(Contains %placeholder1 %word1)");

    auto queries = metta_correlation_queries(args.correlation_query_expressions);
    ASSERT_EQ(queries.size(), 1u);
    ASSERT_EQ(queries[0].size(), 1u);
    EXPECT_EQ(queries[0][0], "(Contains $placeholder1 $word1)");

    auto replacements = metta_correlation_replacements(args.correlation_replacement_groups);
    ASSERT_EQ(replacements.size(), 1u);
    EXPECT_EQ(replacements[0].at("placeholder1").to_string(), "$sentence1");

    auto mappings = metta_correlation_mappings(args.correlation_mapping_groups);
    ASSERT_EQ(mappings.size(), 1u);
    ASSERT_EQ(mappings[0].size(), 1u);
    EXPECT_EQ(mappings[0][0].first.to_string(), "$sentence1");
    EXPECT_EQ(mappings[0][0].second.to_string(), "$word1");
}

TEST(EvolutionMettaParser, parse_multiple_correlation_slots_as_s_expr_lists) {
    EvolutionMettaArgs args;
    string metta_arg =
        "((query (Contains %sentence1 (Word \"bbb\"))) "
        "(ff count_letter) "
        "(cq ((Contains %placeholder1 %word1) (Contains %placeholder1 %word2))) "
        "(cr (((placeholder1 sentence1)) ((placeholder1 sentence1)))) "
        "(cm (((sentence1 word1)) ((sentence1 word2)))))";
    ASSERT_TRUE(try_parse_evolution_metta_arg(metta_arg, args));
    ASSERT_EQ(args.correlation_query_expressions.size(), 2u);
    EXPECT_EQ(args.correlation_query_expressions[0], "(Contains %placeholder1 %word1)");
    EXPECT_EQ(args.correlation_query_expressions[1], "(Contains %placeholder1 %word2)");
    ASSERT_EQ(args.correlation_replacement_groups.size(), 2u);
    EXPECT_EQ(args.correlation_replacement_groups[0][0].first, "placeholder1");
    EXPECT_EQ(args.correlation_replacement_groups[1][0].second, "sentence1");
    ASSERT_EQ(args.correlation_mapping_groups.size(), 2u);
    EXPECT_EQ(args.correlation_mapping_groups[0][0].first, "sentence1");
    EXPECT_EQ(args.correlation_mapping_groups[1][0].second, "word2");

    auto queries = metta_correlation_queries(args.correlation_query_expressions);
    ASSERT_EQ(queries.size(), 2u);
    EXPECT_EQ(queries[0][0], "(Contains $placeholder1 $word1)");
    auto replacements = metta_correlation_replacements(args.correlation_replacement_groups);
    ASSERT_EQ(replacements.size(), 2u);
    EXPECT_EQ(replacements[0].at("placeholder1").to_string(), "$sentence1");
    auto mappings = metta_correlation_mappings(args.correlation_mapping_groups);
    ASSERT_EQ(mappings.size(), 2u);
    EXPECT_EQ(mappings[1][0].second.to_string(), "$word2");
}

TEST(EvolutionMettaParser, canonical_param_key_accepts_name_or_alias) {
    EXPECT_EQ(canonical_evolution_param_key("query"), "query");
    EXPECT_EQ(canonical_evolution_param_key("ff"), "fitness-function-tag");
    EXPECT_EQ(canonical_evolution_param_key("fitness-function-tag"), "fitness-function-tag");
    EXPECT_EQ(canonical_evolution_param_key("cq"), "correlation-queries");
    EXPECT_EQ(canonical_evolution_param_key("correlation-queries"), "correlation-queries");
    EXPECT_EQ(canonical_evolution_param_key("cr"), "correlation-replacements");
    EXPECT_EQ(canonical_evolution_param_key("cm"), "correlation-mappings");
    EXPECT_TRUE(canonical_evolution_param_key("unknown").empty());
}

TEST(EvolutionMettaParser, parse_mixed_aliases_and_full_names) {
    EvolutionMettaArgs args;
    string metta_arg =
        "((query (Contains %sentence1 (Word \"bbb\"))) "
        "(fitness-function-tag count_letter) "
        "(cq (Contains %placeholder1 %word1)) "
        "(correlation-replacements (((placeholder1 sentence1)))) "
        "(cm (((sentence1 word1)))))";
    ASSERT_TRUE(try_parse_evolution_metta_arg(metta_arg, args));
    EXPECT_EQ(args.query, "(Contains %sentence1 (Word \"bbb\"))");
    EXPECT_EQ(args.fitness_function_tag, "count_letter");
    ASSERT_EQ(args.correlation_query_expressions.size(), 1u);
    EXPECT_EQ(args.correlation_query_expressions[0], "(Contains %placeholder1 %word1)");
}

TEST(EvolutionMettaParser, parse_labeled_evolution_arg_with_full_slot_names) {
    EvolutionMettaArgs args;
    string metta_arg =
        "((query (Contains %sentence1 (Word \"bbb\"))) "
        "(fitness-function-tag count_letter) "
        "(correlation-queries (Contains %placeholder1 %word1)) "
        "(correlation-replacements (((placeholder1 sentence1)))) "
        "(correlation-mappings (((sentence1 word1)))))";
    ASSERT_TRUE(try_parse_evolution_metta_arg(metta_arg, args));
    EXPECT_EQ(args.query, "(Contains %sentence1 (Word \"bbb\"))");
    EXPECT_EQ(args.fitness_function_tag, "count_letter");
}

TEST(EvolutionMettaParser, bare_query_expression_is_not_labeled_form) {
    EvolutionMettaArgs args;
    string metta_arg = "((Contains %sentence1 (Word \"bbb\")) (ff count_letter))";
    EXPECT_FALSE(try_parse_evolution_metta_arg(metta_arg, args));
}

TEST(EvolutionMettaParser, plain_query_arg_is_not_labeled_form) {
    EvolutionMettaArgs args;
    EXPECT_FALSE(try_parse_evolution_metta_arg("(Similarity $a $b)", args));
}

TEST(BusCommandRouter, get_and_set_params) {
    set<string> commands = {ServiceBus::BUS_COMMAND_ROUTER};
    ServiceBus::initialize_statics(commands, 40500, 40599);

    auto router_processor = make_shared<BusCommandRouterProcessor>();
    ServiceBus router_bus("localhost:40510");
    router_bus.register_processor(router_processor);
    Utils::sleep(500);

    ServiceBus client_bus("localhost:40511", "localhost:40510");
    Utils::sleep(500);

    auto get_proxy = make_shared<RouterTestProxy>("get", "params");
    client_bus.issue_bus_command(get_proxy);
    Utils::sleep(1000);
    EXPECT_FALSE(get_proxy->params_response.empty());
    EXPECT_TRUE(get_proxy->finished());

    auto set_proxy = make_shared<RouterTestProxy>("set", "param max_answers 777");
    client_bus.issue_bus_command(set_proxy);
    Utils::sleep(1000);
    EXPECT_EQ(set_proxy->set_param_ack, "Parameter updated: 'max_answers': 777");
    EXPECT_TRUE(set_proxy->finished());

    auto get_after_set = make_shared<RouterTestProxy>("get", "params");
    client_bus.issue_bus_command(get_after_set);
    Utils::sleep(1000);
    EXPECT_NE(get_after_set->params_response.find("max_answers: 777"), string::npos);
    EXPECT_TRUE(get_after_set->finished());

    auto set_one = make_shared<RouterTestProxy>("set", "param max_answers 1");
    client_bus.issue_bus_command(set_one);
    Utils::sleep(1000);
    EXPECT_EQ(set_one->set_param_ack, "Parameter updated: 'max_answers': 1");

    auto get_one = make_shared<RouterTestProxy>("get", "params");
    client_bus.issue_bus_command(get_one);
    Utils::sleep(1000);
    EXPECT_NE(get_one->params_response.find("max_answers: 1"), string::npos);
    EXPECT_EQ(get_one->params_response.find("max_answers: true"), string::npos);
}

TEST(BusCommandRouter, set_param_rejects_unknown_and_type_mismatch) {
    set<string> commands = {ServiceBus::BUS_COMMAND_ROUTER};
    ServiceBus::initialize_statics(commands, 40800, 40899);

    auto router_processor = make_shared<BusCommandRouterProcessor>();
    ServiceBus router_bus("localhost:40810");
    router_bus.register_processor(router_processor);
    Utils::sleep(500);

    ServiceBus client_bus("localhost:40811", "localhost:40810");
    Utils::sleep(500);

    auto set_unknown = make_shared<RouterTestProxy>("set", "param does_not_exist hello");
    client_bus.issue_bus_command(set_unknown);
    Utils::sleep(1000);
    EXPECT_TRUE(set_unknown->error_flag);
    EXPECT_NE(set_unknown->error_message.find("Unknown parameter: 'does_not_exist'"), string::npos);
    EXPECT_TRUE(set_unknown->set_param_ack.empty());

    auto set_wrong_type = make_shared<RouterTestProxy>("set", "param max_answers not_a_number");
    client_bus.issue_bus_command(set_wrong_type);
    Utils::sleep(1000);
    EXPECT_TRUE(set_wrong_type->error_flag);
    EXPECT_NE(set_wrong_type->error_message.find("unsigned_int"), string::npos);

    auto set_bool_with_int = make_shared<RouterTestProxy>("set", "param unique_assignment_flag 7");
    client_bus.issue_bus_command(set_bool_with_int);
    Utils::sleep(1000);
    EXPECT_TRUE(set_bool_with_int->error_flag);
    EXPECT_NE(set_bool_with_int->error_message.find("bool"), string::npos);

    auto set_bool_value = make_shared<RouterTestProxy>("set", "param unique_assignment_flag false");
    client_bus.issue_bus_command(set_bool_value);
    Utils::sleep(1000);
    EXPECT_FALSE(set_bool_value->error_flag);
    EXPECT_EQ(set_bool_value->set_param_ack, "Parameter updated: 'unique_assignment_flag': false");
}

TEST(BusCommandRouter, params_isolated_per_peer) {
    set<string> commands = {ServiceBus::BUS_COMMAND_ROUTER};
    ServiceBus::initialize_statics(commands, 40700, 40799);

    auto router_processor = make_shared<BusCommandRouterProcessor>();
    ServiceBus router_bus("localhost:40710");
    router_bus.register_processor(router_processor);
    Utils::sleep(500);

    ServiceBus client_a("localhost:40711", "localhost:40710");
    ServiceBus client_b("localhost:40712", "localhost:40710");
    Utils::sleep(500);

    auto set_a = make_shared<RouterTestProxy>("set", "param max_answers 111");
    client_a.issue_bus_command(set_a);
    Utils::sleep(1000);

    auto set_b = make_shared<RouterTestProxy>("set", "param max_answers 222");
    client_b.issue_bus_command(set_b);
    Utils::sleep(1000);

    auto get_a = make_shared<RouterTestProxy>("get", "params");
    client_a.issue_bus_command(get_a);
    Utils::sleep(1000);
    EXPECT_NE(get_a->params_response.find("max_answers: 111"), string::npos);
    EXPECT_EQ(get_a->params_response.find("max_answers: 222"), string::npos);

    auto get_b = make_shared<RouterTestProxy>("get", "params");
    client_b.issue_bus_command(get_b);
    Utils::sleep(1000);
    EXPECT_NE(get_b->params_response.find("max_answers: 222"), string::npos);
    EXPECT_EQ(get_b->params_response.find("max_answers: 111"), string::npos);
}

TEST(BusCommandRouter, forward_query_preserves_caller_identity) {
    set<string> commands = {ServiceBus::BUS_COMMAND_ROUTER, ServiceBus::PATTERN_MATCHING_QUERY};
    ServiceBus::initialize_statics(commands, 40600, 40699);

    auto query_processor = make_shared<CaptureForwardProcessor>();
    shared_ptr<ServiceBus> query_bus = make_shared<ServiceBus>("localhost:40610");
    query_bus->register_processor(query_processor);
    Utils::sleep(500);

    shared_ptr<ServiceBus> router_bus = make_shared<ServiceBus>("localhost:40611", "localhost:40610");
    auto router_processor = make_shared<BusCommandRouterProcessor>(router_bus);
    router_bus->register_processor(router_processor);
    Utils::sleep(1000);

    ServiceBus client_bus("localhost:40612", "localhost:40611");
    Utils::sleep(500);

    auto query_proxy = make_shared<RouterTestProxy>("query", "(Similarity $a $b)");
    client_bus.issue_bus_command(query_proxy);
    Utils::sleep(1500);

    EXPECT_TRUE(query_proxy->routed_flag);
    // Router issues the downstream query (requestor is the router bus node).
    EXPECT_EQ(query_processor->last_requestor_id, "localhost:40611");
    // Answers return to the router's query proxy port, not the client's listen address.
    EXPECT_NE(query_processor->last_proxy_node_id, query_proxy->my_id());
    bool found_query = false;
    for (const auto& arg : query_processor->last_packed_args) {
        if (arg.find("Similarity") != string::npos) {
            found_query = true;
            break;
        }
    }
    EXPECT_TRUE(found_query);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new BusCommandRouterTestEnvironment());
    return RUN_ALL_TESTS();
}
