#include <iostream>
#include <string>
#include <signal.h>
#include <cstring>

#include "agent.h"
using namespace link_creation_agent;
using namespace std;


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
    
    if ((argc < 5) || (strcmp(argv[1], "client") != 0 && strcmp(argv[1], "server") != 0))
    {
        cerr << help << endl;
        exit(1);
    }

    string type = argv[1];
    string config_path = argv[2];
    
    if (type == "client")
    {
        cerr << "Client not implemented yet" << endl;
    }
    else
    {
        auto server = new LinkCreationAgent(config_path);
        server->run();
    }

    return 0;
}
