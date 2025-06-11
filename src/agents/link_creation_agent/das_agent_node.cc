#include "das_agent_node.h"

#include "Logger.h"

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
    try {
        this->send("create_link", request, this->server_id);
    } catch (const exception& e) {
        LOG_ERROR("DasAgentNode::create_link: Exception: " << e.what());
    }
}