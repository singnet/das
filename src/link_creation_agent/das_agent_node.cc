#include "das_agent_node.h"

using namespace das_agent;
using namespace std;
using namespace distributed_algorithm_node;

DasAgentNode::DasAgentNode(const string& node_id, const string& server_id) : StarNode(node_id) {
    this->node_id = node_id;
    this->server_id = server_id;
}

DasAgentNode::~DasAgentNode() { DistributedAlgorithmNode::graceful_shutdown(); }

void DasAgentNode::create_link(vector<string>& request) {
    // TODO add send link creation request
    cout << "Creating link" << endl;
    for (string token : request) {
        cout << token << " ";
    }
    cout << endl;

    send("create_link", request, this->server_id);
}