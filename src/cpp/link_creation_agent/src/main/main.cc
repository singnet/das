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
    if ((argc < 3) || (strcmp(argv[2], "client") != 0 && strcmp(argv[2], "server") != 0))
    {
        cerr << "Usage: " << argv[0] << "\nSuported args:\n--config_file\t path to config file\n--type\t 'client' or 'server'" << endl;
        exit(1);
    }

    string config_path = argv[1];
    string type = argv[2];
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
