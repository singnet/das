#include <signal.h>

#include <cstring>
#include <iostream>
#include <string>

#include "das_link_creation_node.h"
using namespace link_creation_agent;
using namespace std;

void ctrl_c_handler(int) {
    std::cout << "Stopping client..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

/**
 * @brief Main function
 * Reads the config file and starts the DAS NODE client/server
 * @param argc Number of arguments
 * @param argv Arguments
 * @returns Returns 0 if the program runs successfully
 */
int main(int argc, char* argv[]) {
    string help = R""""(
    Usage: link_creation_agent CLIENT_HOST:CLIENT_PORT SERVER_HOST:SERVER_PORT REQUEST+

    Requests must be in the following format:
    QUERY, LINK_TEMPLATE, MAX_RESULTS, REPEAT, CONTEXT, UPDATE_ATTENTION_BROKER
    MAX_RESULTS and REPEAT are optional, the default value for MAX_RESULTS is 1000 and for REPEAT is 1
    )"""";

    if ((argc < 4)) {
        cerr << help << endl;
    }
    signal(SIGINT, &ctrl_c_handler);

    string client_id = string(argv[1]);
    string server_id = string(argv[2]);

    vector<string> request;
    for (int i = 3; i < argc; i++) {
        request.push_back(argv[i]);
    }

    auto client = new LinkCreationNode(client_id, server_id);
    client->send_message(request);

    return 0;
}
