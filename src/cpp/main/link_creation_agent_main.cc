#include <iostream>
#include <string>
#include <signal.h>
#include <cstring>

#include "agent.h"
using namespace link_creation_agent;
using namespace std;

void ctrl_c_handler(int) {
    std::cout << "Stopping server..." << std::endl;
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
int main(int argc, char *argv[])
{
    string help = R""""(
    Usage: link_creation_agent --config_file <path> --type <client/server>
    Suported args:
    --type          client or server values are accepted
    --config_file   path to config file (server only)
    --help          print this message
    --client_id     client id (client only)
    --request       request to be sent to the server (client only)

    Requests must be in the following format:
    QUERY, LINK_TEMPLATE, MAX_RESULTS, REPEAT
    MAX_RESULTS and REPEAT are optional, the default value for MAX_RESULTS is 1000 and for REPEAT is 1
    )"""";
    
    if ((argc < 4) )
    {
        cerr << help << endl;
        for (auto arg = 0; arg < argc; arg++)
        {
            cerr << "arg[" << arg << "] = " << argv[arg] << endl;
        }
        exit(1);
    }
    signal(SIGINT, &ctrl_c_handler);
    string type = argv[2];
    string config_path = argv[4];
    
    if (type == "client")
    {
        cerr << "Client not implemented yet" << endl;
    }
    else
    {
        cout << "Starting server" << endl;
        auto server = new LinkCreationAgent(config_path);
        server->run();
    }

    return 0;
}
