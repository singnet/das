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
    std::cout << "Stopping query engine server..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <port> BUS_IP:PORT TOKENS+" << endl;
        exit(1);
    }

    string server_id = "0.0.0.0:" + string(argv[1]);

    AtomDBSingleton::init();
    ServiceBusSingleton::init(server_id, argv[2]);
    FitnessFunctionRegistry::initialize_statics();

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    vector<string> query;
    for (int i = 3; i < argc; i++) {
        query.push_back(argv[i]);
    }

    auto proxy = make_shared<QueryEvolutionProxy>(query, "unit_test", "evolution_client");
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

    return 0;
}
