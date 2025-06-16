#include "QueryEvolutionProcessor.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBus.h"
#include "Utils.h"
#include "gtest/gtest.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace evolution;

TEST(QueryEvolution, queries) {
    ServiceBus::initialize_statics({}, 54501, 55000);

    string peer1_id = "localhost:33801";
    string peer2_id = "localhost:33802";

    ServiceBus* server_bus = new ServiceBus(peer1_id);
    Utils::sleep(500);
    server_bus->register_processor(make_shared<QueryEvolutionProcessor>());
    Utils::sleep(500);
    ServiceBus* client_bus = new ServiceBus(peer2_id, peer1_id);
    Utils::sleep(500);

    Utils::sleep(2000);
}
