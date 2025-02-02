#include <iostream>
#include <string>
#include <signal.h>

#include "Utils.h"
#include "DASNode.h"
#include "SentinelServerNode.h"

using namespace std;
using namespace client_spawner;

void ctrl_c_handler(int) {
    std::cout << "Stopping SentinelServerNode..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <PORT>" << endl;
        exit(1);
    }

    string server_id = "localhost:" + string(argv[1]);

    signal(SIGINT, &ctrl_c_handler);
        
    SentinelServerNode server(server_id);

    cout << "#############################     Sentinel Server ON     ##################################" << endl;

    do {
        Utils::sleep(1000);
    } while (true);
    
    return 0;
}
