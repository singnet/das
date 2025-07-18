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

TEST(QueryEvolution, protected_methods) {
    TestConfig::load_environment();
    AtomDBSingleton::init();

    string peer1_id = "localhost:33801";
    string peer2_id = "localhost:33802";
    ServiceBusSingleton::init(peer1_id, "", 64000, 64999);
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

    QueryEvolutionProxy proxy(query, {}, {}, "unit_test", "query_evolution_test");
}
