#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBProcessor.h"
#include "AtomDBSingleton.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atomdb;
using namespace atomdb_broker;
using namespace service_bus;

void ctrl_c_handler(int) {
    LOG_INFO("Stopping AtomDB server...");
    LOG_INFO("Done.");
    exit(0);
}

int main(int argc, char* argv[]) {
    if ((argc != 4) && (argc != 5)) {
        cerr << "AtomDB Broker Server" << endl;
        cerr << "Usage: " << argv[0]
             << " <ip:port> <start_port:end_port> <peer_ip:peer_port> [--use-mork]" << endl;
        exit(1);
    }

    if ((argc == 5) && (string(argv[4]) == string("--use-mork"))) {
        AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::MORKDB);
    } else {
        AtomDBSingleton::init();
        auto db = dynamic_pointer_cast<RedisMongoDB>(AtomDBSingleton::get_instance());

        string tokens = "LINK_TEMPLATE Expression 3 VARIABLE v1 VARIABLE v2 VARIABLE v3";
        vector<vector<string>> index_entries = {{"v1", "v2", "v3"},
                                                {"*", "v2", "v3"},
                                                {"v1", "*", "v3"},
                                                {"*", "*", "v3"},
                                                {"v1", "v2", "*"},
                                                {"*", "v2", "*"},
                                                {"v1", "*", "*"},
                                                {"*", "*", "*"}};
        db->add_pattern_index_schema(tokens, index_entries);

        string tokens_2 = "LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION VARIABLE v1 VARIABLE v2";
        vector<vector<string>> index_entries_2 = {{"_", "*", "*"}, {"_", "v1", "*"}, {"_", "*", "v2"}};
        db->add_pattern_index_schema(tokens_2, index_entries_2);

        string tokens_3 = "LINK_TEMPLATE Expression 2 NODE Symbol PREDICATE VARIABLE v1";
        vector<vector<string>> index_entries_3 = {{"_", "*"}, {"_", "v1"}};
        db->add_pattern_index_schema(tokens_3, index_entries_3);

        string tokens_4 = "LINK_TEMPLATE Expression 2 NODE Symbol CONCEPT VARIABLE v1";
        vector<vector<string>> index_entries_4 = {{"_", "*"}, {"_", "v1"}};
        db->add_pattern_index_schema(tokens_4, index_entries_4);
    }

    string host_id = string(argv[1]);
    string peer_id = string(argv[3]);
    auto ports_range = Utils::parse_ports_range(argv[2]);

    LOG_INFO("Starting AtomDB Broker server with id: " + host_id);

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    ServiceBusSingleton::init(host_id, peer_id, ports_range.first, ports_range.second);
    shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
    service_bus->register_processor(make_shared<AtomDBProcessor>());

    LOG_INFO(
        "#############################     REQUEST QUEUE EMPTY     ##################################");

    do {
        Utils::sleep();
    } while (true);

    return 0;
}
