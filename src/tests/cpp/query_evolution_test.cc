#include "FitnessFunctionRegistry.h"
#include "QueryEvolutionProcessor.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBus.h"
#include "Utils.h"
#include "gtest/gtest.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace evolution;

TEST(QueryEvolution, queries) {
    ServiceBus::initialize_statics({}, 54501, 55000);
    FitnessFunctionRegistry::initialize_statics();

    string peer1_id = "localhost:33801";
    string peer2_id = "localhost:33802";

    ServiceBus* server_bus = new ServiceBus(peer1_id);
    Utils::sleep(500);
    server_bus->register_processor(make_shared<QueryEvolutionProcessor>());
    Utils::sleep(500);
    ServiceBus* client_bus = new ServiceBus(peer2_id, peer1_id);
    Utils::sleep(500);

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

    shared_ptr<QueryEvolutionProxy> proxy(
        new QueryEvolutionProxy(query, "unit_test", "query_evolution_test_queriees"));
    proxy->set_unique_assignment_flag(false);

    client_bus->issue_bus_command(proxy);
    Utils::sleep(1000);
    // EXPECT_FALSE(proxy->error_flag);
}
