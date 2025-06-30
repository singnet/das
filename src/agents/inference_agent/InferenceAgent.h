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
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>


#include "InferenceRequest.h"
#include "InferenceRequestValidator.h"
#include "ThreadPool.h"
#include "InferenceProxy.h"
#include "LinkCreationRequestProxy.h"
#include "LCAQueue.h"

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
    /**
     * @brief Stop the agent, stop the agent thread, stop the inference_node_server, stop the
     * link_creation_node_client, stop the das_client, stop the distributed_inference_control_client.
     */
    void stop();

    /**
     * @brief Process inference request.
     * @param inference_request The inference request to be processed.
     */
    void process_inference_request(const vector<string>& request, const string& request_id);

    /**
     * @brief Process inference abort request.
     * @param request_id The inference request to be processed.
     */
    void process_inference_abort_request(const string& request_id);

   private:
    /**
     * @brief Send link creation request to the link creation agent.
     * @param inference_request The inference request to be sent to the link creation agent.
     * @param is_stop_request Whether the request is a stop request.
     */
    void send_link_creation_request(shared_ptr<InferenceRequest> inference_request);
    /**
     * @brief Send stop link creation request to the link creation agent.
     * @param inference_request The inference request to be sent to the link creation agent.
     */
    void send_stop_link_creation_request(const string& request_id);
    /**
     * @brief Send stop link creation request to the link creation agent.
     * @param inference_request The inference request to be sent to the link creation agent.
     */
    void send_distributed_inference_control_request(const std::string& client_node_id);
    // /**
    //  * @brief Send stop link creation request to the link creation agent.
    //  * @param inference_request The inference request to be sent to the link creation agent.
    //  */
    // void parse_config(const string& config_path);
    // /**
    //  * @brief Get the next iterator id.
    //  * @return The next iterator id.
    //  */
    // const string get_next_iterator_id();
    // const string get_next_inference_request_id();
    /**
     * @brief Build inference request from the given inference request.
     * @param request The inference request to be built.
     * @return The built inference request.
     */
    shared_ptr<InferenceRequest> build_inference_request(const vector<string>& request);
    // Private variables
    InferenceRequestValidator inference_request_validator;
    std::vector<std::string> get_link_creation_request();
    int max_proof_length_limit = 10;
    thread* agent_thread = nullptr;
    bool is_stoping = false;
    std::mutex agent_mutex;
    std::unordered_map<std::string, std::shared_ptr<InferenceRequest>>
        iterator_link_creation_request_map;  // iterator_id, link_creation_request
    unsigned long long inference_request_id = 0;
    Queue<shared_ptr<InferenceRequest>> inference_request_queue;
    unordered_map<string, vector<shared_ptr<LinkCreationRequestProxy>>> link_creation_proxy_map;

    static const std::string PROOF_OF_IMPLICATION_OR_EQUIVALENCE;
    static const std::string PROOF_OF_IMPLICATION;
    static const std::string PROOF_OF_EQUIVALENCE;
};
}  // namespace inference_agent
