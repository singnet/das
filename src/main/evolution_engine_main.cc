#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "QueryEvolutionProcessor.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;
using namespace evolution;
using namespace service_bus;

void ctrl_c_handler(int) {
    // std::cout << "Stopping evolution engine server..." << std::endl;
    std::cout << "Cleaning GRPC buffers..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <port>" << endl;
        exit(1);
    }

    string server_id = "0.0.0.0:" + string(argv[1]);
    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    AtomDBSingleton::init();
    ServiceBusSingleton::init(server_id);
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
