/**
 * @file agent.h
 * @brief InferenceAgent class is the main
 * class of the inference agent. It is responsible for
 * receiving inference request from the client, sending
 * link creation request to the link creation agent, and
 * listening to the distributed inference control agent
 * to finish the inference and send request to stop link
 * creation.
 */

#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "das_agent_node.h"
#include "distributed_inference_control_agent_node.h"
#include "inference_agent_node.h"
#include "inference_iterator.h"
#include "inference_request.h"
#include "inference_request_validator.h"
#include "link_creation_agent_node.h"
#include "thread_pool.h"

using namespace distributed_algorithm_node;
using namespace link_creation_agent;
using namespace das_agent;
using namespace distributed_inference_control_agent;

namespace inference_agent {
class InferenceAgent {
   public:
    InferenceAgent(const string& config_path);
    InferenceAgent(InferenceAgentNode* inference_node_server,
                   LinkCreationAgentNode* link_creation_node_client,
                   DasAgentNode* das_client,
                   DistributedInferenceControlAgentNode* distributed_inference_control_client);
    ~InferenceAgent();
    /**
     * @brief Start the agent, receive inference request from the client, send link_creation
     * request to the link_creation_agent, listen when distributed inference control agent
     * finish the inference, send request to stop link creation.
     */
    void run();
    /**
     * @brief Stop the agent, stop the agent thread, stop the inference_node_server, stop the
     * link_creation_node_client, stop the das_client, stop the distributed_inference_control_client.
     */
    void stop();

   private:
   /**
    * @brief Send link creation request to the link creation agent.
    * @param inference_request The inference request to be sent to the link creation agent.
    * @param is_stop_request Whether the request is a stop request.
    */
    void send_link_creation_request(shared_ptr<InferenceRequest> inference_request,
                                    bool is_stop_request);
    /**
     * @brief Send stop link creation request to the link creation agent.
     * @param inference_request The inference request to be sent to the link creation agent.
     */
    void send_stop_link_creation_request(shared_ptr<InferenceRequest> inference_request);
    /**
     * @brief Send stop link creation request to the link creation agent.
     * @param inference_request The inference request to be sent to the link creation agent.
     */
    void send_distributed_inference_control_request(const std::string& client_node_id);
    /**
     * @brief Send stop link creation request to the link creation agent.
     * @param inference_request The inference request to be sent to the link creation agent.
     */
    void parse_config(const string& config_path);
    /**
     * @brief Get the next iterator id.
     * @return The next iterator id.
     */
    const string get_next_iterator_id();
    // Private variables
    const string get_next_inference_request_id();
    InferenceRequestValidator inference_request_validator;
    std::vector<std::string> get_link_creation_request();
    std::string inference_node_id;
    std::string das_client_id;
    std::string das_server_id;
    std::string distributed_inference_control_node_id;
    std::string distributed_inference_control_node_server_id;
    std::string link_creation_agent_server_id;
    std::string link_creation_agent_client_id;
    std::string inference_node_server_host;
    std::string inference_node_server_port;
    uint16_t current_iterator_id = 0;
    int iterator_pool_size = 10;
    thread* agent_thread;
    bool is_stoping = false;
    std::mutex agent_mutex;
    std::unordered_map<std::string, std::shared_ptr<InferenceRequest>>
        iterator_link_creation_request_map;  // iterator_id, link_creation_request
    std::vector<shared_ptr<InferenceIterator<InferenceAgentNode>>> inference_iterators;
    DasAgentNode* das_client;
    InferenceAgentNode* inference_node_server;
    LinkCreationAgentNode* link_creation_node_client;
    DistributedInferenceControlAgentNode* distributed_inference_control_client;
    unsigned long long inference_request_id = 0;

    static const std::string PROOF_OF_IMPLICATION_OR_EQUIVALENCE;
    static const std::string PROOF_OF_IMPLICATION;
    static const std::string PROOF_OF_EQUIVALENCE;
};
}  // namespace inference_agent
