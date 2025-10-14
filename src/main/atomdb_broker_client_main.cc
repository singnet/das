#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"
#include "AtomDBBrokerProxy.h"

using namespace std;
using namespace service_bus;
using namespace atom_space;
using namespace atomdb;
using namespace atomdb_broker;


int main(int argc, char* argv[]) {
    if (argc < 6) {
        cerr << "Usage: " << argv[0]
             << " <client_ip:client_port> <server_ip:server_port> <start_port:end_port> <action> <tokens+> "
             << endl;
        exit(1);
    }

    string client_id = string(argv[1]);
    string server_id = string(argv[2]);
    auto ports_range = Utils::parse_ports_range(argv[3]);
    string action = string(argv[4]);
    vector<string> tokens;
    for (int i = 5; i < argc; i++) {
        tokens.push_back(argv[i]);
    }

    AtomDBSingleton::init();
    ServiceBusSingleton::init(client_id, server_id, ports_range.first, ports_range.second);
    auto proxy = make_shared<AtomDBBrokerProxy>(action, tokens);
    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);

    while (!proxy->finished()) {
        Utils::sleep();
    }

    return 0;
}
