#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "FitnessFunctionRegistry.h"
#include "QueryAnswer.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

using namespace std;
using namespace query_engine;
using namespace service_bus;
using namespace atomdb;
using namespace evolution;

void ctrl_c_handler(int) {
    cout << "Stopping query engine server..." << endl;
    cout << "Done." << endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        cerr << "Usage: " << argv[0] << " <port> <start_port:end_port> BUS_IP:PORT TOKENS+" << endl;
        exit(1);
    }

    string server_id = "0.0.0.0:" + string(argv[1]);
    auto ports_range = Utils::parse_ports_range(argv[2]);
    AtomDBSingleton::init();
    ServiceBusSingleton::init(server_id, argv[3], ports_range.first, ports_range.second);
    FitnessFunctionRegistry::initialize_statics();

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    vector<string> query;
    for (int i = 4; i < argc; i++) {
        query.push_back(argv[i]);
    }

    /*
    auto proxy = make_shared<QueryEvolutionProxy>(query, "evolution_client", "unit_test");
    auto bus = ServiceBusSingleton::get_instance();
    shared_ptr<QueryAnswer> query_answer;
    bus->issue_bus_command(proxy);

    int count = 1;
    while (!proxy->finished()) {
        if ((query_answer = proxy->pop()) == NULL) {
            Utils::sleep();
        } else {
            cout << count << ": " << query_answer->to_string() << endl;
        }
        count++;
    }
    if (count == 1) {
        cout << "No match for query" << endl;
    }
    */

    return 0;
}
