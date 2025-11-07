#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "AttentionBrokerClient.h"
#include "FitnessFunctionRegistry.h"
#include "QueryEvolutionProcessor.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atomdb;
using namespace evolution;
using namespace service_bus;
using namespace attention_broker;

void ctrl_c_handler(int) {
    LOG_INFO("Stopping evolution engine server...");
    LOG_INFO("Done.");
    exit(0);
}

int main(int argc, char* argv[]) {
    if ((argc < 5) && (argc != 6)) {
        cerr << "Usage: " << argv[0]
             << " <hostname>:<port> <start_port:end_port> BUS_IP:PORT AB_ip:AB_port [--use-mork]"
             << endl;
        exit(1);
    }
    AttentionBrokerClient::set_server_address(string(argv[4]));

    if ((argc == 6) && (string(argv[5]) == string("--use-mork"))) {
        AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::MORKDB);
    } else {
        AtomDBSingleton::init();
    }

    string server_id = string(argv[1]);
    auto ports_range = Utils::parse_ports_range(argv[2]);
    ServiceBusSingleton::init(server_id, argv[3], ports_range.first, ports_range.second);

    FitnessFunctionRegistry::initialize_statics();

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    LOG_INFO("Starting evolution engine server with id: " + server_id);
    shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
    service_bus->register_processor(make_shared<QueryEvolutionProcessor>());

    LOG_INFO(
        "#############################     REQUEST QUEUE EMPTY     ##################################");
    do {
        Utils::sleep(1000);
    } while (true);
    return 0;
}
