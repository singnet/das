/**
 * @file distributed_inference_control_agent_node.h
 * @brief Distributed inference control agent node
 */

#pragma once

#include "StarNode.h"
using namespace distributed_algorithm_node;

namespace distributed_inference_control_agent {
class DistributedInferenceControlAgentNode : public StarNode {
   public:
    DistributedInferenceControlAgentNode(const std::string& node_id, const std::string& server_id);
    ~DistributedInferenceControlAgentNode();
};
}  // namespace distributed_inference_control_agent
