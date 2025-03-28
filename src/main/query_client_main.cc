#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "DASNode.h"
#include "HandlesAnswer.h"
#include "QueryAnswer.h"
#include "RemoteIterator.h"
#include "Utils.h"

#define MAX_QUERY_ANSWERS ((unsigned int) 1)

using namespace std;

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

    signal(SIGINT, &ctrl_c_handler);
    vector<string> query;
    for (int i = 4; i < argc; i++) {
        query.push_back(argv[i]);
    }

    DASNode client(client_id, server_id);
    QueryAnswer* query_answer;
    unsigned int count = 0;
    RemoteIterator<HandlesAnswer>* response =
        client.pattern_matcher_query(query, "", update_attention_broker);
    while (!response->finished()) {
        if ((query_answer = response->pop()) == NULL) {
            Utils::sleep();
        } else {
            cout << query_answer->to_string() << endl;
            if (++count == MAX_QUERY_ANSWERS) {
                break;
            }
            delete query_answer;
        }
    }
    if (count == 0) {
        cout << "No match for query" << endl;
    }

    delete response;
    return 0;
}
