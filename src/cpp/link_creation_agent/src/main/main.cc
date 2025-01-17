#include <iostream>
#include <string>
#include <signal.h>

#include "core.h"
#include "das_server_node.h"
#include "das_link_creation_node.h"
#include "das_query_node.h"

int main(int argc, char* argv[]){
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << "\nSuported args:\n--config_file\t path to config file\n--type\t 'client' or 'server'" << std::endl;
        exit(1);
    }
    auto das_server_node = new das::ServerNode("", "");
    auto das_link_creation_node = new lca::LinkCreationNode("");
    auto das_query_agent_client = new query_node::QueryNode("", false);
    auto das_query_agent_server = new query_node::QueryNode("", true);

    auto server = new link_creation_agent::LinkCreationAgent(argv[1], 
    das_query_agent_client, 
    das_query_agent_server, 
    das_link_creation_node,
    das_server_node );
    server->run();
    // Read config file
    // Start DAS NODE client/server
    // loop forever
    return 0;
}