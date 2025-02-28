#include "distributed_inference_control_agent_node.h"

#include <iostream>
using namespace distributed_inference_control_agent;

DistributedInferenceControlAgentNode::DistributedInferenceControlAgentNode(const std::string& node_id,
                                                                           const std::string& server_id)
    : StarNode(node_id, server_id) {
    is_server = true;
}

DistributedInferenceControlAgentNode::~DistributedInferenceControlAgentNode() {
    DistributedAlgorithmNode::graceful_shutdown();
}

void DistributedInferenceControlAgentNode::send_inference_control_request(
    std::vector<std::string> inference_control_request) {
    for (auto& token : inference_control_request) {
        cout << token << " ";
    }
    cout << endl;
}
