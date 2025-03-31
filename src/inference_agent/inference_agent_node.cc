#include "inference_agent_node.h"

#include "dummy_message.h"

using namespace inference_agent;
using namespace std;

const std::string InferenceAgentNode::CREATE_INFERENCE = "create_inference";
const std::string InferenceAgentNode::INFERENCE_ANSWER = "inference_answer";
const std::string InferenceAgentNode::DISTRIBUTED_INFERENCE_FINISHED = "evolution_finished";

InferenceAgentNode::InferenceAgentNode(const std::string& node_id) : StarNode(node_id) {}

InferenceAgentNode::InferenceAgentNode(const std::string& node_id, const std::string& server_id)
    : StarNode(node_id, server_id) {}

InferenceAgentNode::InferenceAgentNode(const std::string& node_id, MessageBrokerType messaging_backend)
    : StarNode(node_id, server_id, messaging_backend) {}

InferenceAgentNode::InferenceAgentNode(const std::string& node_id,
                                       const std::string& server_id,
                                       MessageBrokerType messaging_backend)
    : StarNode(node_id, server_id, messaging_backend) {}

InferenceAgentNode::~InferenceAgentNode() {
    shutting_down = true;
    DistributedAlgorithmNode::graceful_shutdown();
}

bool InferenceAgentNode::is_answers_finished() { return answers_finished; }

bool InferenceAgentNode::is_answers_empty() { return answers_queue.empty(); }

vector<string> InferenceAgentNode::pop_answer() { return answers_queue.dequeue(); }

void InferenceAgentNode::set_answers_finished() { answers_finished = true; }

void InferenceAgentNode::send_message(std::vector<std::string> args) {
    send(CREATE_INFERENCE, args, server_id);
}

void InferenceAgentNode::add_request(vector<string> request) { answers_queue.enqueue(request); }

std::shared_ptr<Message> InferenceAgentNode::message_factory(std::string& command,
                                                             std::vector<std::string>& args) {
    shared_ptr<Message> message = StarNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (command == CREATE_INFERENCE) {
        return make_shared<CreateInferenceMessage>(command, args);
    }
    if (command == DISTRIBUTED_INFERENCE_FINISHED) {
        return make_shared<DistributedInferenceFinishedMessage>(command, args);
    }
#ifdef DEBUG
    cout << "Command not recognized" << endl;
#endif
    return make_shared<DummyMessage>(command, args);
}

CreateInferenceMessage::CreateInferenceMessage(std::string command, std::vector<std::string> args) {
    this->command = command;
    this->args = args;
}

void CreateInferenceMessage::act(std::shared_ptr<MessageFactory> node) {
    auto inference_node = dynamic_pointer_cast<InferenceAgentNode>(node);
    inference_node->add_request(this->args);
}

DistributedInferenceFinishedMessage::DistributedInferenceFinishedMessage(std::string command,
                                                                         std::vector<std::string> args) {
    this->command = command;
    this->args = args;
}

void DistributedInferenceFinishedMessage::act(std::shared_ptr<MessageFactory> node) {
    auto inference_node = dynamic_pointer_cast<InferenceAgentNode>(node);
    inference_node->add_request(this->args);
    inference_node->set_answers_finished();
}
