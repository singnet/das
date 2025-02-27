/**
 * @file das_agent_node.h
 * @brief DAS Node client to send add atom requests
 */
#pragma once
#include "DistributedAlgorithmNode.h"
#include "StarNode.h"
#include "queue.h"

using namespace distributed_algorithm_node;

namespace das_agent {
class DasAgentNode : public StarNode {
   public:
    /**
     * @brief Client constructor
     * @param node_id ID of this node in the network.
     * @param server_id ID of a server.
     */
    DasAgentNode(const string& node_id, const string& server_id);
    /**
     * Destructor
     */
    ~DasAgentNode();
    /**
     * @brief Create a link
     * @param request Request to create a link
     */
    void create_link(vector<string>& request);

   private:
    const string CREATE_ATOM = "create_atom";  // DAS Node command
};
}  // namespace das
