#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryAnswer.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

#define MAX_QUERY_ANSWERS ((unsigned int) 1)

using namespace std;
using namespace query_engine;
using namespace service_bus;

void ctrl_c_handler(int) {
    std::cout << "Stopping query engine server..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        cerr << "Usage: " << argv[0]
             << " CLIENT_HOST:CLIENT_PORT SERVER_HOST:SERVER_PORT UPDATE_ATTENTION_BROKER QUERY_TOKEN+ "
                "(hosts are supposed to be public IPs or known hostnames)"
             << endl;
        exit(1);
    }

    string client_id = string(argv[1]);
    string server_id = string(argv[2]);
    bool update_attention_broker = (string(argv[3]) == "true" || string(argv[3]) == "1");

    ServiceBusSingleton::init(client_id, server_id);

    // check if argv[4] is a number which is the max number of query answers
    // if not, set it to MAX_QUERY_ANSWERS
    int max_query_answers = 0;
    size_t tokens_start_position = 4;
    try {
        max_query_answers = stol(argv[4]);
        if (max_query_answers <= 0) {
            cout << "max_query_answers cannot be 0 or negative" << endl;
            exit(1);
        }
        tokens_start_position = 5;
    } catch (const std::exception& e) {
        max_query_answers = MAX_QUERY_ANSWERS;
    }
    cout << "Using max_query_answers: " << max_query_answers << endl;

    signal(SIGINT, &ctrl_c_handler);
    vector<string> query;
    for (int i = tokens_start_position; i < argc; i++) {
        query.push_back(argv[i]);
    }

    shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
    shared_ptr<PatternMatchingQueryProxy> proxy =
        make_shared<PatternMatchingQueryProxy>(query, "", update_attention_broker);
    service_bus->issue_bus_command(proxy);
    shared_ptr<QueryAnswer> query_answer;
    int count = 0;
    while (!proxy->finished()) {
        if ((query_answer = proxy->pop()) == NULL) {
            Utils::sleep();
        } else {
            cout << query_answer->to_string() << endl;
            if (++count == max_query_answers) {
                break;
            }
        }
    }
    if (count == 0) {
        cout << "No match for query" << endl;
    }

    return 0;
}
