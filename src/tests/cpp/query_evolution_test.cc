#include "AtomDBAPITypes.h"
#include "AtomDBSingleton.h"
#include "FitnessFunctionRegistry.h"
#include "Logger.h"
#include "PatternMatchingQueryProcessor.h"
#include "QueryEvolutionProcessor.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"
#include "TestAtomDBJsonConfig.h"
#include "TestSystemParams.h"
#include "Utils.h"
#include "gtest/gtest.h"

using das_test::init_test_system_parameters_singleton;

using namespace atomdb;
using namespace evolution;

class TestProcessor : public QueryEvolutionProcessor {
   public:
    TestProcessor() {}
    ~TestProcessor() {}
    void sample_population(shared_ptr<QueryEvolutionProxy> proxy,
                           vector<std::pair<shared_ptr<QueryAnswer>, float>>& population) {
        shared_ptr<StoppableThread> monitor;
        QueryEvolutionProcessor::sample_population(monitor, proxy, population);
    }
};

class TestFitnessFunction : public FitnessFunction {
   public:
    float eval(shared_ptr<QueryAnswer> query_answer) override { return 1; }
};

TEST(QueryEvolution, protected_methods) {
    AtomDBSingleton::init(test_atomdb_json_config());
    init_test_system_parameters_singleton();

    string peer1_id = "localhost:40043";
    string peer2_id = "localhost:40044";
    ServiceBusSingleton::init(peer1_id, "", 40300, 40399);
    FitnessFunctionRegistry::initialize_statics();
    shared_ptr<ServiceBus> query_bus = ServiceBusSingleton::get_instance();
    query_bus->register_processor(make_shared<PatternMatchingQueryProcessor>());
    Utils::sleep(1000);

    auto processor = make_shared<TestProcessor>();
    shared_ptr<ServiceBus> bus = make_shared<ServiceBus>(peer2_id, peer1_id);
    Utils::sleep(1000);
    bus->register_processor(processor);

    vector<string> query = {"LINK_TEMPLATE",
                            "Expression",
                            "3",
                            "NODE",
                            "Symbol",
                            "Similarity",
                            "VARIABLE",
                            "v1",
                            "VARIABLE",
                            "v2"};

    QueryEvolutionProxy proxy1(query, {}, {}, {}, "query_evolution_test", "unit_test");
    EXPECT_EQ(proxy1.compute_fitness(make_shared<QueryAnswer>("blah", 0.0)), 0.0);
    EXPECT_EQ(proxy1.compute_fitness(make_shared<QueryAnswer>("blah", 0.5)), 0.5);
    EXPECT_EQ(proxy1.compute_fitness(make_shared<QueryAnswer>("blah", 1.0)), 1.0);
    QueryEvolutionProxy proxy2(query,
                               {},
                               {},
                               {},
                               "query_evolution_test",
                               FitnessFunctionRegistry::REMOTE_FUNCTION,
                               make_shared<TestFitnessFunction>());
    EXPECT_EQ(proxy2.compute_fitness(make_shared<QueryAnswer>("blah", 0.0)), 1.0);
    EXPECT_EQ(proxy2.compute_fitness(make_shared<QueryAnswer>("blah", 0.5)), 1.0);
    EXPECT_EQ(proxy2.compute_fitness(make_shared<QueryAnswer>("blah", 1.0)), 1.0);
    vector<string> tokens;
    proxy2.tokenize(tokens);
    QueryEvolutionProxy proxy3;
    proxy3.untokenize(tokens);
    EXPECT_FALSE(proxy1.is_fitness_function_remote());
    EXPECT_FALSE(proxy2.is_fitness_function_remote());
    EXPECT_TRUE(proxy3.is_fitness_function_remote());
    EXPECT_THROW(proxy3.compute_fitness(make_shared<QueryAnswer>("blah", 0.0)), runtime_error);
}

TEST(QueryEvolution, proxy_object) {
    QueryAnswerElement e1(0);
    QueryAnswerElement e2(1);
    QueryAnswerElement e3("s1");
    QueryAnswerElement e4(0, 1);
    QueryAnswerElement e5(QueryAnswerElement::ALL_HANDLES);
    QueryAnswerElement e6(QueryAnswerElement::ALL_VARIABLE_VALUES);
    QueryAnswerElement e7(QueryAnswerElement::ALL_PATH_HANDLES);
    QueryAnswerElement e8(QueryAnswerElement::EVERYTHING);
    QueryAnswerElement e9(0, 1, 2, false);
    QueryAnswerElement e10(1, 0, 1, true);
    QueryAnswerElement e11(2, 0, 2, true);
    QueryAnswerElement e12(0, 0, 1, false, false, false);
    QueryAnswerElement e13(0, 0, 1, false, false, true);
    QueryAnswerElement e14(0, 0, 1, false, true, false);
    QueryAnswerElement e15(0, 0, 1, false, true, true);
    QueryAnswerElement e16(0, 0, 1, true, false, false);
    QueryAnswerElement e17(0, 0, 1, true, false, true);
    QueryAnswerElement e18(0, 0, 1, true, true, false);
    QueryAnswerElement e19(0, 0, 1, true, true, true);
    QueryEvolutionProxy proxy({"t0", "t1"},
                              {{"tc00"}, {}, {"tc10", "tc11"}},
                              {{{"h1", e1}}, {}, {{"h2", e2}, {"h3", e3}}},
                              {{{e2, e1}, {e1, e2}, {e1, e3}, {e4, e5}, {e6, e7}, {e7, e8}, {e9, e10}, {e9, e11}, {e12, e13}, {e14, e15}, {e16, e17}, {e18, e19}}},
                              "query_evolution_test",
                              "unit_test");

    EXPECT_EQ(
        proxy.to_string(),
        "{BaseQueryProxy: {context: query_evolution_test, tokens: [t0, t1], BaseProxy: {parameters: "
        "{allow_incomplete_chain_path: false, attention_correlation: 0, attention_update: 0, "
        "elitism_rate: 0.010000, "
        "max_answers: 0, max_bundle_size: 1000, "
        "max_generations: 100, populate_metta_mapping: false, population_size: 1000, selection_rate: "
        "0.100000, unique_assignment_flag: false, use_link_template_cache: "
        "false, use_metta_as_query_tokens: false}}}, fitness_function: unit_test, correlation_queries: "
        "[[tc00], [], [tc10, tc11]], correlation_replacements: [{{h1, _0}}, {}, {{h2, _1}, {h3, $s1}}], "
        "correlation_mappings: [[(_1, _0), (_0, _1), (_0, $s1), (^0_1, _*), ($*, ^*), (^*, *), (>0_1_2, <1_0_1), (>0_1_2, <2_0_2), (>0_0_1, >$0_0_1), (>^0_0_1, >^$0_0_1), (<0_0_1, <$0_0_1), (<^0_0_1, <^$0_0_1)]]}");
    vector<string> tokens1, tokens2, tokens3;
    proxy.tokenize(tokens1);
    tokens2 = tokens1;
    QueryEvolutionProxy proxy2;
    proxy2.untokenize(tokens2);
    proxy2.tokenize(tokens3);
    cout << "tokens1: " << Utils::join(tokens1) << endl;
    cout << "tokens3: " << Utils::join(tokens3) << endl;
    EXPECT_EQ(tokens1, tokens3);
}
