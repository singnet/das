#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "QueryAnswer.h"
#include "ServiceBusSingleton.h"
#include "QueryEvolutionProxy.h"
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
    if (argc < 3) {
        cerr << "Usage: " << argv[0]
             << " CLIENT_HOST:CLIENT_PORT SERVER_HOST:SERVER_PORT QUERY_TOKEN+ "
                "(hosts are supposed to be public IPs or known hostnames)"
             << endl;
        exit(1);
    }

    AtomDBSingleton::init();

    string client_id = string(argv[1]);
    string server_id = string(argv[2]);

    ServiceBusSingleton::init(client_id, server_id, 54000, 54500);

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    vector<string> query;
    for (int i = 3; i < argc; i++) {
        query.push_back(argv[i]);
    }

    auto proxy = make_shared<QueryEvolutionProxy>(query, "unit_test", "evolution_client");
    shared_ptr<QueryAnswer> query_answer;
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
