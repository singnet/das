/**
 * @file agent.h
 */

#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <map>

#include "das_link_creation_node.h"
#include "inference_node.h"
#include "inference_iterator.h"

using namespace distributed_algorithm_node;
using namespace link_creation_agent;

namespace inference_agent {
class InferenceAgent {
   public:
    InferenceAgent();
    ~InferenceAgent();
    /**
     * @brief Start the agent, receive inference request from the client, send link_creation
     * request to the link_creation_agent, listen when distributed inference control agent
     * finish the inference, send request to stop link creation.
     */
    void run();
    void stop();

   private:
    void send_link_creation_request();
    void send_stop_link_creation_request();
    void send_distributed_inference_control_request(const std::string& client_node_id);
    std::vector<std::string> get_link_creation_request();
    std::string inference_node_id;
    std::string das_client_id;
    std::string das_server_id;
    std::string distributed_inference_control_node_id;
    std::string distributed_inference_control_node_server_id;
    std::string link_creation_agent_server_id;
    std::string link_creation_agent_client_id;
    thread* agent_thread;
    bool is_stoping = false;
    std::mutex agent_mutex;
    InferenceNode* inference_node_server;
    LinkCreationNode* link_creation_node_client;
    std::unordered_map<std::string, std::string> iterator_link_creation_request_map; // iterator_id, link_creation_request
    std::vector<shared_ptr<InferenceIterator<InferenceNode>>> inference_iterators;
    // DasAgentNode* das_client;
    // DistributedInferenceControlNode* distributed_inference_control_node;

};
}  // namespace inference_agent
