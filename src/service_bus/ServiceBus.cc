#include "ServiceBus.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace service_bus;

string ServiceBus::PATTERN_MATCHING_QUERY = "pattern_matching_query";
set<string> ServiceBus::SERVICE_LIST;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

ServiceBus::Node::Node(const string& id,
                       shared_ptr<BusNode::Bus> bus,
                       const set<string>& node_commands,
                       const string& known_peer)
    : BusNode(id, *bus, node_commands, known_peer, MessageBrokerType::GRPC) {
    this->bus = bus;
}

ServiceBus::ServiceBus(const string& host_id, const string& known_peer) {
    this->next_request_serial = 1;
    this->bus = shared_ptr<BusNode::Bus>(new BusNode::Bus());
    for (auto command : ServiceBus::SERVICE_LIST) {
        this->bus->add(command);
    }
    this->bus_node =
        shared_ptr<ServiceBus::Node>(new ServiceBus::Node(host_id, this->bus, {}, known_peer));
    Utils::sleep(500);
}

ServiceBus::~ServiceBus() {
    LOG_DEBUG("Destroying ServiceBus " << bus_node->node_id());
    this->bus_node->graceful_shutdown();
    LOG_DEBUG("Destroying ServiceBus " << bus_node->node_id() << " DONE");
}

void ServiceBus::initialize_statics(const set<string>& commands,
                                    unsigned int port_lower,
                                    unsigned int port_upper) {
    LOG_INFO("BUS static initialization");
    if (commands.size() > 0) {
        for (auto command : commands) {
            SERVICE_LIST.insert(command);
        }
    } else {
        SERVICE_LIST.insert(PATTERN_MATCHING_QUERY);
    }
    for (string command : SERVICE_LIST) {
        LOG_INFO("BUS command: <" << command << ">");
    }
    PortPool::initialize_statics(port_lower, port_upper);
}

// -------------------------------------------------------------------------------------------------
// Public API

void ServiceBus::register_processor(shared_ptr<BusCommandProcessor> processor) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->bus_node->take_ownership(processor->commands);
    this->bus_node->processor = processor;
}

void ServiceBus::issue_bus_command(shared_ptr<BusCommandProxy> proxy) {
    lock_guard<mutex> semaphore(this->api_mutex);
    LOG_INFO(bus_node->node_id() << " is issuing BUS command <" << proxy->command << ">");
    proxy->requestor_id = this->bus_node->node_id();
    proxy->serial = this->next_request_serial++;
    proxy->proxy_port = PortPool::get_port();
    if (proxy->proxy_port == 0) {
        Utils::error("No port is available to start bus command proxy");
    } else {
        proxy->setup_proxy_node();
        vector<string> args;
        args.push_back(proxy->requestor_id);
        args.push_back(to_string(proxy->serial));
        args.push_back(proxy->proxy_node->node_id());
        for (auto arg : proxy->args) {
            args.push_back(arg);
        }
        this->bus_node->send_bus_command(proxy->command, args);
    }
}

// -------------------------------------------------------------------------------------------------
// Messages

shared_ptr<Message> ServiceBus::Node::message_factory(string& command, vector<string>& args) {
    shared_ptr<Message> message = BusNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (this->bus->contains(command)) {
        return shared_ptr<Message>(new BusCommandMessage(command, args));
    }
    return shared_ptr<Message>{};
}

ServiceBus::BusCommandMessage::BusCommandMessage(const string& command, const vector<string>& args) {
    this->command = command;
    this->args = args;
}

void ServiceBus::BusCommandMessage::act(shared_ptr<MessageFactory> node) {
    auto service_bus_node = dynamic_pointer_cast<ServiceBus::Node>(node);
    LOG_INFO("Command: <" << this->command
                          << "> delivered to bus element: " << service_bus_node->node_id());
    if (service_bus_node->processor->check_command(this->command)) {
        shared_ptr<BusCommandProxy> proxy = service_bus_node->processor->factory_empty_proxy();
        proxy->proxy_port = PortPool::get_port();
        if (proxy->proxy_port == 0) {
            Utils::error("No port is available to start bus command proxy");
        }
        if (this->args.size() < 2) {
            Utils::error("Invalid BUS command syntax");
        }
        proxy->requestor_id = this->args[0];
        proxy->serial = stoi(this->args[1]);
        string id = service_bus_node->node_id();
        string my_host = id.substr(0, id.find(":"));
        string my_id = my_host + ":" + to_string(proxy->proxy_port);
        proxy->setup_proxy_node(my_id, this->args[2]);
        proxy->command = this->command;
        for (unsigned int i = 3; i < this->args.size(); i++) {
            proxy->args.push_back(this->args[i]);
        }
        service_bus_node->processor->run_command(proxy);
    } else {
        Utils::error("Processor is not registered to process command: " + this->command);
    }
}
