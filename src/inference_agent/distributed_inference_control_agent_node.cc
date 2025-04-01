#include "distributed_inference_control_agent_node.h"

#include <iostream>
using namespace distributed_inference_control_agent;

const std::string DistributedInferenceControlAgentNode::EVOLUTION_REQUEST_COMMAND = "evolution_request";

DistributedInferenceControlAgentNode::DistributedInferenceControlAgentNode(const std::string& node_id,
                                                                           const std::string& server_id)
    : StarNode(node_id, server_id) {
    this->node_id = node_id;
    this->server_id = server_id;
    is_server = true;
}

DistributedInferenceControlAgentNode::~DistributedInferenceControlAgentNode() {
    DistributedAlgorithmNode::graceful_shutdown();
}

void DistributedInferenceControlAgentNode::send_inference_control_request(
    std::vector<std::string> inference_control_request, std::string response_node_id) {
    std::vector<std::string> request;
    request.push_back(response_node_id);
#ifdef DEBUG
    for (auto& token : inference_control_request) {
        cout << token << " ";
        request.push_back(token);
    }
    cout << endl;
#endif
    this->send_message(request);
}

void DistributedInferenceControlAgentNode::send_message(std::vector<std::string> args) {
    send(EVOLUTION_REQUEST_COMMAND, args, this->server_id);
}
