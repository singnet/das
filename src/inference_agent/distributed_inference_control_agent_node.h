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
    virtual void send_inference_control_request(std::vector<std::string> inference_control_request);
};
}  // namespace distributed_inference_control_agent
