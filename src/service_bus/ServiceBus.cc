#include "ServiceBus.h"

using namespace service_bus;


// --------------------------------------------------------------------------------
// Constructors and destructors

ServiceBus::Node(const string& id,
                 const BusNode::Bus& bus,
                 const set<string>& node_commands,
                 const string& known_peer)
    : BusNode(id, bus, node_commands, known_peer, MessageBrokerType::RAM) {

    this->bus = bus;
}

virtual ~Node();


// --------------------------------------------------------------------------------
// Public methods

ServiceBus::ServiceBus() {
}

ServiceBus::~ServiceBus() {
}

// --------------------------------------------------------------------------------
// Messages

shared_ptr<Message> ServiceBus::Node::message_factory(string& command, 
                                                           vector<string>& args) {

    shared_ptr<Message> message = BusNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (this->bus.contains(command)) {
        return shared_ptr<Message>(new BusCommand(command, args));
    }
    return shared_ptr<Message>{};
}

ServiceBus::BusCommand::BusCommand(const string& command, 
                                   const vector<string>& args) {

    this->command = command;
    this->args = args;
}

void ServiceBus::BusCommand::act(shared_ptr<MessageFactory> node) {
    auto node = dynamic_pointer_cast<>(ServiceBus::Node);
    TODO
}


