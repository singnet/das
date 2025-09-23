#include "AtomDBSingleton.h"
#include "AtomSpace.h"
#include "FitnessFunctionRegistry.h"
#include "PatternMatchingQueryProcessor.h"
#include "QueryEvolutionProcessor.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"
#include "TestConfig.h"
#include "Utils.h"
#include "gtest/gtest.h"

// #define LOG_LEVEL DEBUG_LEVEL
// #include "Logger.h"

using namespace evolution;

class TestProcessor : public QueryEvolutionProcessor {
   public:
    AtomSpace atom_space;
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
    TestConfig::load_environment();
    AtomDBSingleton::init();

    string peer1_id = "localhost:40043";
    string peer2_id = "localhost:40044";
    ServiceBusSingleton::init(peer1_id, "", 40300, 40399);
    FitnessFunctionRegistry::initialize_statics();
    shared_ptr<ServiceBus> query_bus = ServiceBusSingleton::get_instance();
    query_bus->register_processor(make_shared<PatternMatchingQueryProcessor>());
    Utils::sleep(1000);

    auto processor = make_shared<TestProcessor>();
    ServiceBus* bus = new ServiceBus(peer2_id, peer1_id);
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
    QueryEvolutionProxy proxy({"t0", "t1"},
                              {{"tc00"}, {}, {"tc10", "tc11"}},
                              {{{"h1", e1}}, {}, {{"h2", e2}, {"h3", e3}}},
                              {{e2, e1}, {e1, e2}, {e1, e3}},
                              "query_evolution_test",
                              "unit_test");

    EXPECT_EQ(
        proxy.to_string(),
        "{BaseQueryProxy: {context: query_evolution_test, tokens: [t0, t1], BaseProxy: {parameters: "
        "{attention_update_flag: false, elitism_rate: 0.010000, max_answers: 0, max_bundle_size: 1000, "
        "max_generations: 100, populate_metta_mapping: false, population_size: 1000, selection_rate: "
        "0.100000, total_attention_tokens: 100, unique_assignment_flag: false, use_link_template_cache: "
        "false, use_metta_as_query_tokens: false}}}, fitness_function: unit_test, correlation_queries: "
        "[[tc00], [], [tc10, tc11]], correlation_replacements: [{{h1, _0}}, {}, {{h2, _1}, {h3, $s1}}], "
        "correlation_mappings: [(_1, _0), (_0, _1), (_0, $s1)]}");
    vector<string> tokens1, tokens2, tokens3;
    proxy.tokenize(tokens1);
    tokens2 = tokens1;
    QueryEvolutionProxy proxy2;
    proxy2.untokenize(tokens2);
    proxy2.tokenize(tokens3);
    EXPECT_EQ(tokens1, tokens3);
}
