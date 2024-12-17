#include <iostream>
#include <string>

#include <signal.h>

#include "DASNode.h"
#include "RemoteIterator.h"
#include "QueryAnswer.h"
#include "AtomDBSingleton.h"
#include "Utils.h"

#define MAX_QUERY_ANSWERS ((unsigned int) 1000)

using namespace std;

void ctrl_c_handler(int) {
    std::cout << "Stopping query engine server..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {

    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " CLIENT_HOST:CLIENT_PORT SERVER_HOST:SERVER_PORT QUERY_TOKEN+ (hosts are supposed to be public IPs or known hostnames)" << endl;
        exit(1);
    }

    string client_id = string(argv[1]);
    string server_id = string(argv[2]);

    signal(SIGINT, &ctrl_c_handler);
    vector<string> query;
    for (int i = 3; i < argc; i++) {
        query.push_back(argv[i]);
    }

    DASNode client(client_id, server_id);
    QueryAnswer *query_answer;
    unsigned int count = 0;
    RemoteIterator *response = client.pattern_matcher_query(query);
    while (! response->finished()) {
        if ((query_answer = response->pop()) == NULL) {
            Utils::sleep();
        } else {
            cout << query_answer->to_string() << endl;
            if (++count == MAX_QUERY_ANSWERS) {
                break;
            }
        }
    }
    if (count == 0) {
        cout << "No match for query" << endl;
    }

    delete response;
    return 0;
}
