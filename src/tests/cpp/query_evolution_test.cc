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

TEST(QueryEvolution, proxy_object) {
    QueryEvolutionProxy proxy(
        {"t0", "t1"} ,
        {{"tc00"}, {}, {"tc10", "tc11"}},
        {{{1, 0}}, {}, {{1, 1}, {2, "v1"}}}, 
        "query_evolution_test", "unit_test");

    cout << "XXXXXXXXX " << proxy.to_string() << " XXXXXXXXXXXXXXXXXXXXXX" << endl;
    FAIL();
    //EXPECT_EQ(proxy.to_string(), "
}

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

    QueryEvolutionProxy proxy1(query, {}, {}, "query_evolution_test", "unit_test");
    EXPECT_EQ(proxy1.compute_fitness(make_shared<QueryAnswer>("blah", 0.0)), 0.0);
    EXPECT_EQ(proxy1.compute_fitness(make_shared<QueryAnswer>("blah", 0.5)), 0.5);
    EXPECT_EQ(proxy1.compute_fitness(make_shared<QueryAnswer>("blah", 1.0)), 1.0);
    QueryEvolutionProxy proxy2(query,
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
