#include "distributed_inference_control_agent_node.h"

using namespace distributed_inference_control_agent;

DistributedInferenceControlAgentNode::DistributedInferenceControlAgentNode(const std::string& node_id, const std::string& server_id) : StarNode(node_id, server_id) {
    is_server = true;
} 

DistributedInferenceControlAgentNode::~DistributedInferenceControlAgentNode() {
    DistributedAlgorithmNode::graceful_shutdown();
}


