#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "FitnessFunctionRegistry.h"
#include "QueryEvolutionProcessor.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;
using namespace evolution;
using namespace service_bus;

void ctrl_c_handler(int) {
    cout << "Stopping evolution engine server..." << endl;
    cout << "Done." << endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <port> <start_port:end_port> BUS_IP:PORT" << endl;
        exit(1);
    }

    string server_id = "0.0.0.0:" + string(argv[1]);
    auto ports_range = Utils::parse_ports_range(argv[2]);
    AtomDBSingleton::init();
    ServiceBusSingleton::init(server_id, argv[3], ports_range.first, ports_range.second);

    FitnessFunctionRegistry::initialize_statics();

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
    service_bus->register_processor(make_shared<QueryEvolutionProcessor>());

    cout
        << "#############################     REQUEST QUEUE EMPTY     ##################################"
        << endl;
    do {
        Utils::sleep(1000);
    } while (true);
    return 0;
}
