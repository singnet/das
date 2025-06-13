/**
 * @file inference_node.h
 */

#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "LCAQueue.h"
#include "Message.h"
#include "StarNode.h"

using namespace distributed_algorithm_node;
using namespace std;

namespace inference_agent {
class InferenceAgentNode : public StarNode {
   public:
    /**
     * @brief Construct a new Inference Agent Node object
     *
     * @param node_id
     */
    InferenceAgentNode(const std::string& node_id);
    /**
     * @brief Construct a new Inference Agent Node object
     *
     * @param node_id
     * @param server_id
     */
    InferenceAgentNode(const std::string& node_id, const std::string& server_id);
    /**
     * @brief Construct a new Inference Agent Node object
     *
     * @param node_id
     * @param messaging_backend
     */
    InferenceAgentNode(const std::string& node_id, MessageBrokerType messaging_backend);
    /**
     * @brief Construct a new Inference Agent Node object
     *
     * @param node_id
     * @param server_id
     * @param messaging_backend
     */
    InferenceAgentNode(const std::string& node_id,
                       const std::string& server_id,
                       MessageBrokerType messaging_backend);
    /**
     * @brief Destroy the Inference Agent Node object
     */
    ~InferenceAgentNode();

    /**
     * @brief Check if the answers queue is finished
     */
    virtual bool is_answers_finished();
    /**
     * @brief Check if the answers queue is empty
     */
    virtual bool is_answers_empty();
    /**
     * @brief Pop an answer from the answers queue
     */
    virtual vector<string> pop_answer();
    /**
     * @brief Set the answers queue as finished
     */
    virtual void set_answers_finished();
    /**
     * @brief Add a request to the request queue
     * @param request Request to be added
     */
    virtual void add_request(vector<string> request);

    /**
     * @brief Send a message to the agent node
     * @param args Arguments of the message
     */
    virtual void send_message(std::vector<std::string> args);

    /**
     * @brief Handle incoming messages
     */
    virtual std::shared_ptr<Message> message_factory(std::string& command,
                                                     std::vector<std::string>& args);

    static const std::string CREATE_INFERENCE;
    static const std::string INFERENCE_ANSWER;
    static const std::string DISTRIBUTED_INFERENCE_FINISHED;

   private:
    std::string local_host;
    unsigned int next_query_port;
    unsigned int first_query_port;
    unsigned int last_query_port;
    std::thread* agent_node_thread;
    bool is_stoping = false;
    std::mutex agent_node_mutex;
    Queue<vector<string>> answers_queue;
    bool answers_finished = false;
    bool shutting_down = false;
};

class CreateInferenceMessage : public Message {
   public:
    CreateInferenceMessage(std::string command, std::vector<std::string> args);
    void act(std::shared_ptr<MessageFactory> node) override;
    string command;
    vector<string> args;
};

class DistributedInferenceFinishedMessage : public Message {
   public:
    DistributedInferenceFinishedMessage(std::string command, std::vector<std::string> args);
    void act(std::shared_ptr<MessageFactory> node) override;
    string command;
    vector<string> args;
};

}  // namespace inference_agent
