#include <signal.h>

#include <cstring>
#include <iostream>
#include <string>

#include "inference_agent_node.h"

using namespace inference_agent;
using namespace std;

void ctrl_c_handler(int) {
    std::cout << "Stopping client..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    string help = R""""(
    Usage: inference_agent CLIENT_HOST:CLIENT_PORT SERVER_HOST:SERVER_PORT REQUEST+
    Requests must be in the following format:
        PROOF_OF_IMPLICATION_OR_EQUIVALENCE <handle1> <handle2> <max proof length>
        PROOF_OF_IMPLICATION <handle1> <handle2> <max proof length>
        PROOF_OF_EQUIVALENCE <handle1> <handle2> <max proof length>

    )"""";

    if (argc < 4) {
        cerr << help << endl;
        exit(1);
    }

    string client_id = string(argv[1]);
    string server_id = string(argv[2]);

    vector<string> request;
    for (int i = 3; i < argc; i++) {
        request.push_back(argv[i]);
    }
    signal(SIGINT, &ctrl_c_handler);
    string config_path = argv[2];
    cout << "Starting inference agent" << endl;
    auto client = new InferenceAgentNode(client_id, server_id);
    client->send_message(request);
    return 0;
}