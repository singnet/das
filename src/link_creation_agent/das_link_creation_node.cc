#include "das_link_creation_node.h"

using namespace link_creation_agent;
using namespace std;
using namespace distributed_algorithm_node;

LinkCreationNode::LinkCreationNode(const string& node_id) : StarNode(node_id) {
    is_server = true;
}

LinkCreationNode::LinkCreationNode(const string& node_id, const string& server_id) : StarNode(node_id, server_id) {
    is_server = false;
}

LinkCreationNode::~LinkCreationNode() {
    shutting_down = true;
    DistributedAlgorithmNode::graceful_shutdown();
}

vector<string> LinkCreationNode::pop_request() { return shared_queue.dequeue(); }

bool LinkCreationNode::is_query_empty() { return shared_queue.empty(); }

bool LinkCreationNode::is_shutting_down() { return shutting_down; }

void LinkCreationNode::add_request(vector<string> request) { shared_queue.enqueue(request); }

shared_ptr<Message> LinkCreationNode::message_factory(string& command, vector<string>& args) {
    cout << "LinkCreationNode::message_factory" << endl;
    cout << command << endl;
    shared_ptr<Message> message = StarNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (command == CREATE_LINK) {
        return make_shared<LinkCreationRequest>(command, args);
    }
    cout << "Command not recognized" << endl;
    return make_shared<DummyMessage>(command, args);
}


void LinkCreationNode::send_message(vector<string> args) {
    cout << "Sending message" << endl;
    send(CREATE_LINK, args, server_id);
}

LinkCreationRequest::LinkCreationRequest(string command, vector<string>& args) {
    this->command = command;
    this->args = args;
}

void LinkCreationRequest::act(shared_ptr<MessageFactory> node) {
    auto link_node = dynamic_pointer_cast<LinkCreationNode>(node);
    string request;
    link_node->add_request(this->args);
}


