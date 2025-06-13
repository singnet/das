#include "EvolutionAgentNode.h"

#include <iostream>

#include "Logger.h"
#include "Utils.h"
using namespace distributed_inference_control_agent;
using namespace commons;

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
    for (auto& token : inference_control_request) {
        request.push_back(token);
    }
    LOG_DEBUG("Sending distributed inference control request ID: " << Utils::join(request, ' '));
    this->send_message(request);
}

void DistributedInferenceControlAgentNode::send_message(std::vector<std::string> args) {
    send(EVOLUTION_REQUEST_COMMAND, args, this->server_id);
}
