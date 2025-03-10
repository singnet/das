/**
 * @file inference_node.h
 */

#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "Message.h"
#include "StarNode.h"
#include "queue.h"

using namespace distributed_algorithm_node;
using namespace std;

namespace inference_agent {
class InferenceAgentNode : public StarNode {
   public:
    InferenceAgentNode(const std::string& node_id);
    InferenceAgentNode(const std::string& node_id, const std::string& server_id);
    ~InferenceAgentNode();

    bool is_answers_finished();
    bool is_answers_empty();
    vector<string> pop_answer();
    void set_answers_finished();
    /**
     * @brief Add a request to the request queue
     * @param request Request to be added
     */
    void add_request(vector<string> request);

    void send_message(std::vector<std::string> args);

    virtual std::shared_ptr<Message> message_factory(std::string& command,
                                                     std::vector<std::string>& args);

    static const std::string CREATE_INFERENCE;
    static const std::string INFERENCE_ANSWER;
    static const std::string DISTRIBUTED_INFERENCE_FINISHED;
    // const std::string CREATE_INFERENCE = "create_inference";
    // const std::string INFERENCE_ANSWER = "inference_answer";
    // const std::string DISTRIBUTED_INFERENCE_FINISHED = "distributed_inference_finished";

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