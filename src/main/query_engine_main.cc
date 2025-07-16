#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "PatternMatchingQueryProcessor.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atomdb;
using namespace query_engine;
using namespace service_bus;

void ctrl_c_handler(int) {
    LOG_INFO("Cleaning GRPC buffers...");
    LOG_INFO("Done.");
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <port> <start_port:end_port>" << endl;
        exit(1);
    }
    string server_id = "0.0.0.0:" + string(argv[1]);
    auto ports_range = Utils::parse_ports_range(argv[2]);
    LOG_INFO("Starting query engine server with id: " + server_id);
    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    AtomDBSingleton::init();
    ServiceBusSingleton::init(server_id, "", ports_range.first, ports_range.second);
    shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
    service_bus->register_processor(make_shared<PatternMatchingQueryProcessor>());

    LOG_INFO(
        "#############################     REQUEST QUEUE EMPTY     ##################################");
    do {
        Utils::sleep(1000);
    } while (true);
    return 0;
}
