#include "LinkCreationAgentNode.h"

#include "Logger.h"

using namespace link_creation_agent;
using namespace std;
using namespace distributed_algorithm_node;

LinkCreationAgentNode::LinkCreationAgentNode(const string& node_id) : StarNode(node_id) {
    is_server = true;
}

LinkCreationAgentNode::LinkCreationAgentNode(const string& node_id, const string& server_id)
    : StarNode(node_id, server_id) {
    is_server = false;
}

LinkCreationAgentNode::~LinkCreationAgentNode() {
    shutting_down = true;
    DistributedAlgorithmNode::graceful_shutdown();
}

vector<string> LinkCreationAgentNode::pop_request() { return shared_queue.dequeue(); }

bool LinkCreationAgentNode::is_query_empty() { return shared_queue.empty(); }

bool LinkCreationAgentNode::is_shutting_down() { return shutting_down; }

void LinkCreationAgentNode::add_request(vector<string> request) { shared_queue.enqueue(request); }

shared_ptr<Message> LinkCreationAgentNode::message_factory(string& command, vector<string>& args) {
    LOG_DEBUG("Processing command: " << command);
    shared_ptr<Message> message = StarNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (command == CREATE_LINK) {
        return make_shared<LinkCreationRequest>(command, args);
    }
    LOG_DEBUG("Command not recognized: " << command);
    return make_shared<DummyMessage>(command, args);
}

void LinkCreationAgentNode::send_message(vector<string> args) {
    LOG_DEBUG("Sending message to" << server_id);
    send(CREATE_LINK, args, server_id);
}

LinkCreationRequest::LinkCreationRequest(string command, vector<string>& args) {
    this->command = command;
    this->args = args;
}

void LinkCreationRequest::act(shared_ptr<MessageFactory> node) {
    auto link_node = dynamic_pointer_cast<LinkCreationAgentNode>(node);
    link_node->add_request(this->args);
}
