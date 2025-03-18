/**
 * @file distributed_inference_control_agent_node.h
 * @brief Distributed inference control agent node
 * is responsible for sending and receiving messages
 * to the distributed inference control agent.
 */

#pragma once

#include <string>
#include <vector>

#include "StarNode.h"

using namespace distributed_algorithm_node;

namespace distributed_inference_control_agent {
class DistributedInferenceControlAgentNode : public StarNode {
   public:
    DistributedInferenceControlAgentNode(const std::string& node_id, const std::string& server_id);
    ~DistributedInferenceControlAgentNode();
    virtual void send_inference_control_request(std::vector<std::string> inference_control_request, std::string response_node_id);
    virtual void send_message(std::vector<std::string> args);
    private:
    const static std::string EVOLUTION_REQUEST_COMMAND;
    std::string node_id;
    std::string server_id;
};
}  // namespace distributed_inference_control_agent
