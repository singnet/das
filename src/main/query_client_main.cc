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

    DASNode client(client_id, server_id);
    QueryAnswer* query_answer;
    int count = 0;
    RemoteIterator<HandlesAnswer>* response =
        client.pattern_matcher_query(query, "", update_attention_broker);
    while (!response->finished()) {
        if ((query_answer = response->pop()) == NULL) {
            Utils::sleep();
        } else {
            cout << query_answer->to_string() << endl;
            if (++count == max_query_answers) {
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
