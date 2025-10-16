#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "AtomDBBrokerProcessor.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atomdb;
using namespace atomdb_broker;
using namespace service_bus;

void ctrl_c_handler(int) {
    LOG_INFO("Stopping AtomDBBroker server...");
    LOG_INFO("Done.");
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "AtomDB Broker Server" << endl;
        cerr << "Usage: " << argv[0]
             << " <ip:port> <start_port:end_port> <peer_ip:peer_port>" << endl;
        exit(1);
    }

    string client_id = string(argv[1]);
    string server_id = string(argv[3]);
    auto ports_range = Utils::parse_ports_range(argv[2]);
    
    LOG_INFO("Starting AtomDB Broker server with id: " + server_id);
    
    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    
    AtomDBSingleton::init();
    ServiceBusSingleton::init(client_id, server_id, ports_range.first, ports_range.second);
    shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
    service_bus->register_processor(make_shared<AtomDBBrokerProcessor>());

    LOG_INFO("#############################     REQUEST QUEUE EMPTY     ##################################");
    
    do {
        Utils::sleep();
    } while (true);
    
    return 0;
}
