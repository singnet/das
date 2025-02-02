#include <iostream>
#include <string>
#include <signal.h>

#include "Utils.h"
#include "DASNode.h"
#include "WorkerClientNode.h"

using namespace std;
using namespace client_spawner;

void ctrl_c_handler(int) {
    std::cout << "Stopping WorkerClientNode..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {

    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <NODE_ID> <SERVER_ID> <DAS_NODE_SERVER_ID> <QUERY_TOKENS+>" << endl;
        exit(1);
    }

    string node_id = string(argv[1]);
    string server_id = string(argv[2]);
    string das_node_server_id = string(argv[3]);

    cout << "node_id: " << node_id << endl;
    cout << "server_id: " << server_id << endl;
    cout << "das_node_server_id: " << das_node_server_id << endl;

    signal(SIGINT, &ctrl_c_handler);

    vector<string> query;

    for (int i = 4; i < argc; i++) {
        query.push_back(argv[i]);
    }

    signal(SIGINT, &ctrl_c_handler);
    
    WorkerClientNode client(node_id, server_id, das_node_server_id);
    
    client.execute(query);
    
    cout << "Worker client '" << node_id << "' done!\n" << endl;
    
    return 0;
}
