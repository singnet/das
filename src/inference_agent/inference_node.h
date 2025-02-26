/**
 * @file inference_node.h
 */

#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "StarNode.h"

using namespace distributed_algorithm_node;

namespace inference_agent {
class InferenceNode : public StarNode {
   public:
    InferenceNode(const std::string& node_id);
    InferenceNode(const std::string& node_id, const std::string& server_id);
    ~InferenceNode();

    bool is_answers_finished();
    bool is_answers_empty();
    string pop_answer();


    void send_message(std::vector<std::string> args);

    virtual std::shared_ptr<Message> message_factory(std::string& command,
                                                     std::vector<std::string>& args);

    const std::string CREATE_INFERENCE = "create_inference";
    const std::string INFERENCE_ANSWER = "inference_answer";
    const std::string DISTRIBUTED_INFERENCE_FINISHED = "distributed_inference_finished";

   private:
    std::string local_host;
    unsigned int next_query_port;
    unsigned int first_query_port;
    unsigned int last_query_port;
    std::thread* agent_node_thread;
    bool is_stoping = false;
    std::mutex agent_node_mutex;
};


class CreateInferenceMessage : public Message {
   public:
    CreateInferenceMessage(std::string command, std::vector<std::string> args);
    ~CreateInferenceMessage();
    void act(std::shared_ptr<MessageFactory> node) override;
};


class InferenceAnswerMessage : public Message {
   public:
    InferenceAnswerMessage(std::string command, std::vector<std::string> args);
    ~InferenceAnswerMessage();
    void act(std::shared_ptr<MessageFactory> node) override;
};


class DistributedInferenceFinishedMessage : public Message {
   public:
    DistributedInferenceFinishedMessage(std::string command, std::vector<std::string> args);
    ~DistributedInferenceFinishedMessage();
    void act(std::shared_ptr<MessageFactory> node) override;
};

}  // namespace inference_agent