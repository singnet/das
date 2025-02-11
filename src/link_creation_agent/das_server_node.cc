#include "das_server_node.h"

using namespace das;
using namespace std;
using namespace distributed_algorithm_node;

DasAgentNode::DasAgentNode(const string& node_id, const string& server_id) : StarNode(node_id) {}

DasAgentNode::~DasAgentNode() { DistributedAlgorithmNode::graceful_shutdown(); }

void DasAgentNode::create_link(vector<string>& request) {
    cout << "Creating link" << endl;
    for (string token : request) {
        cout << token << endl;
    }
}