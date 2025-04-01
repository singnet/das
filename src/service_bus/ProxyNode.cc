#include "ProxyNode.h"

using namespace service_bus;

string ProxyNode::PROXY_COMMAND = "bus_command_proxy";


// -------------------------------------------------------------------------------------------------
// Constructors and destructors

ProxyNode::ProxyNode(const string& node_id) : StarNode(node_id) {
    // SERVER running in BUS command requestor
}

ProxyNode::ProxyNode(const string& node_id, const string& server_id) : StarNode(node_id, server_id) {
    // CLIENT running in BUS command processor
}

ProxyNode::~ProxyNode() {
}

// -------------------------------------------------------------------------------------------------
// API

void ProxyNode::remote_call(const string &command, const vector<string> &args) {

}

// -------------------------------------------------------------------------------------------------
// Messages

shared_ptr<Message> ProxyNode::message_factory(string& command, vector<string>& args) {
    std::shared_ptr<Message> message = DistributedAlgorithmNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (command == ProxyNode::PROXY_COMMAND) {
        return std::shared_ptr<Message>(new ProxyMessage(command, args));
    }
    return std::shared_ptr<Message>{};
}

ProxyMessage::ProxyMessage(const string& command, const vector<string>& args) {
    this->command = command;
    this->args = args;
}

void ProxyMessage::act(shared_ptr<MessageFactory> node) {
    auto proxy_node = dynamic_pointer_cast<ProxyNode>(node);
    proxy_node->remote_call(this->command, this->args);
}
