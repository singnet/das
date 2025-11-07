#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "AtomSpace.h"
#include "QueryAnswer.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

#define MAX_QUERY_ANSWERS ((unsigned int) 1000)

using namespace std;
using namespace query_engine;
using namespace service_bus;
using namespace atom_space;
using namespace atomdb;

void ctrl_c_handler(int) {
    cout << "Stopping query engine server..." << endl;
    cout << "Done." << endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc < 7) {
        cerr << "Usage: " << argv[0]
             << " CLIENT_HOST:CLIENT_PORT SERVER_HOST:SERVER_PORT <START_PORT:END_PORT> "
                "--use-mork|--use-redismongo"
                "UPDATE_ATTENTION_BROKER QUERY_TOKEN+ "
                "(hosts are supposed to be public IPs or known hostnames)"
             << endl;
        exit(1);
    }

    string client_id = string(argv[1]);
    string server_id = string(argv[2]);
    auto ports_range = Utils::parse_ports_range(argv[3]);
    if (string(argv[4]) == string("--use-mork")) {
        AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::MORKDB);
    } else {
        AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::REDIS_MONGODB);
    }
    bool update_attention_broker = (string(argv[5]) == "true" || string(argv[5]) == "1");
    if (update_attention_broker) {
        cerr << "Enforcing update_attention_broker=false regardless the passed parameter" << endl;
    }
    update_attention_broker = false;

    ServiceBusSingleton::init(client_id, server_id, ports_range.first, ports_range.second);

    bool USE_METTA_QUERY = false;

    // check if argv[4] is a number which is the max number of query answers
    // if not, set it to MAX_QUERY_ANSWERS
    int max_query_answers = 0;
    size_t tokens_start_position = 5;
    try {
        max_query_answers = stol(argv[6]);
        if (max_query_answers <= 0) {
            cout << "max_query_answers cannot be 0 or negative" << endl;
            exit(1);
        }
        tokens_start_position = 6;
    } catch (const exception& e) {
        max_query_answers = MAX_QUERY_ANSWERS;
    }
    cout << "Using max_query_answers: " << max_query_answers << endl;

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    vector<string> query;
    for (int i = tokens_start_position; i < argc; i++) {
        cout << "XXXXXXXXXX: " << argv[i] << endl;
        query.push_back(argv[i]);
    }

    auto proxy = make_shared<PatternMatchingQueryProxy>(query, "");
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = update_attention_broker;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = (unsigned int) max_query_answers;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = USE_METTA_QUERY;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = true;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);

    shared_ptr<QueryAnswer> query_answer;
    int count = 0;
    while (!proxy->finished()) {
        if ((query_answer = proxy->pop()) == NULL) {
            Utils::sleep();
        } else {
            cout << query_answer->to_string() << endl;
            for (string handle : query_answer->handles) {
                cout << query_answer->metta_expression[handle] << endl;
            }
            if (++count == max_query_answers) {
                break;
            }
        }
    }
    cout << count << " answers" << endl;
    if (count == 0) {
        cout << "No match for query" << endl;
    }

    return 0;
}
