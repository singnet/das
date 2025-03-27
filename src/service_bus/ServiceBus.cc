#include "ServiceBus.h"

#include "Utils.h"

using namespace service_bus;

set<string> ServiceBus::SERVICE_LIST;

// --------------------------------------------------------------------------------
// Constructors and destructors

ServiceBus::Node::Node(const string& id,
                       shared_ptr<BusNode::Bus> bus,
                       const set<string>& node_commands,
                       const string& known_peer)
    : BusNode(id, *bus, node_commands, known_peer, MessageBrokerType::GRPC) {
    this->bus = bus;
}

ServiceBus::ServiceBus(const string& host_id, const string& known_peer) {
    this->bus = shared_ptr<BusNode::Bus>(new BusNode::Bus());
    for (auto command : ServiceBus::SERVICE_LIST) {
        this->bus->add(command);
    }
    this->bus_node =
        shared_ptr<ServiceBus::Node>(new ServiceBus::Node(host_id, this->bus, {}, known_peer));
}

// --------------------------------------------------------------------------------
// Public methods

void ServiceBus::register_processor(shared_ptr<BusCommandProcessor> processor) {
    this->bus_node->take_ownership(processor->commands);
    this->bus_node->processor = processor;
}

// --------------------------------------------------------------------------------
// Messages

shared_ptr<Message> ServiceBus::Node::message_factory(string& command, vector<string>& args) {
    shared_ptr<Message> message = BusNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (this->bus->contains(command)) {
        return shared_ptr<Message>(new BusCommand(command, args));
    }
    return shared_ptr<Message>{};
}

ServiceBus::BusCommand::BusCommand(const string& command, const vector<string>& args) {
    this->command = command;
    this->args = args;
}

void ServiceBus::BusCommand::act(shared_ptr<MessageFactory> node) {
    auto service_bus_node = dynamic_pointer_cast<ServiceBus::Node>(node);
    if (service_bus_node->processor->check_command(this->command)) {
        service_bus_node->processor->run_command(this->command, this->args);
    } else {
        Utils::error("Processor is now registered to process command: " + this->command);
    }
}
