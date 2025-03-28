#include "das_agent_node.h"
#include "link_creation_console.h"


using namespace das_agent;
using namespace std;
using namespace distributed_algorithm_node;

DasAgentNode::DasAgentNode(const string& node_id, const string& server_id)
    : StarNode(node_id, server_id) {
    this->node_id = node_id;
    this->server_id = server_id;
}

DasAgentNode::~DasAgentNode() { DistributedAlgorithmNode::graceful_shutdown(); }

void DasAgentNode::create_link(vector<string>& request) {
    #ifdef DEBUG
    cout << "Creating link" << endl;
    for (string token : request) {
        cout << token << " ";
    }
    cout << endl;
    #endif
    link_creation_agent::Console::get_instance()->print_metta(request);
    send("create_link", request, this->server_id);
}