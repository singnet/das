#include "BusCommandProxy.h"

#include "PortPool.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace service_bus;

string ProxyNode::PROXY_COMMAND = "bus_command_proxy";
string BusCommandProxy::PEER_ERROR = "peer_error";

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

BusCommandProxy::BusCommandProxy() { this->proxy_port = 0; }

BusCommandProxy::BusCommandProxy(const string& command, const vector<string>& args)
    : command(command), args(args) {}

BusCommandProxy::~BusCommandProxy() {
    if (this->proxy_port != 0) {
        // Return the port to the pool of available ports
        PortPool::return_port(this->proxy_port);
        this->proxy_node->graceful_shutdown();
        if (this->proxy_node->is_server()) {
            // Force BUS command caller to wait for the client while it shuts down.  This
            // reduces the probability of having PORTs in TIME_WAIT mode which would make
            // them unusable for a period of time.
            Utils::sleep(500);
        }
        delete this->proxy_node;
    }
}

ProxyNode::ProxyNode(BusCommandProxy* proxy, const string& node_id) : StarNode(node_id) {
    // SERVER running in BUS command requestor
    this->proxy = proxy;
}

ProxyNode::ProxyNode(BusCommandProxy* proxy, const string& node_id, const string& server_id)
    : StarNode(node_id, server_id) {
    // CLIENT running in BUS command processor
    this->proxy = proxy;
}

ProxyNode::~ProxyNode() {}

// -------------------------------------------------------------------------------------------------
// Proxy API

void BusCommandProxy::setup_proxy_node(const string& client_id, const string& server_id) {
    if (this->proxy_port == 0) {
        Utils::error("Proxy node can't be set up");
    } else {
        if (client_id == "") {
            LOG_DEBUG("ProxyNode on CLIENT");
            // This proxy is running in the requestor
            string id = this->requestor_id;
            string requestor_host = id.substr(0, id.find(":"));
            string requestor_id = requestor_host + ":" + to_string(this->proxy_port);

            LOG_DEBUG("requestor_id: " + requestor_id);

            this->proxy_node = new ProxyNode(this, requestor_id);
        } else {
            LOG_DEBUG("ProxyNode on PROCESSOR");
            // This proxy is running in the processor
            this->proxy_node = new ProxyNode(this, client_id, server_id);
            this->proxy_node->peer_id = server_id;
            LOG_DEBUG("client_id: " << client_id);
            LOG_DEBUG("server_id: " << server_id);
        }
    }
}

void BusCommandProxy::to_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG(this->proxy_node->node_id() << " is issuing proxy command <" << command << ">");
    this->proxy_node->to_remote_peer(command, args);
}

bool BusCommandProxy::from_remote_peer(const string& command, const vector<string>& args) {
    if (command == PEER_ERROR) {
        raise_error(args[0], stoi(args[1]));
        return true;
    } else {
        return false;
    }
}

const string& BusCommandProxy::get_command() { return this->command; }

const vector<string>& BusCommandProxy::get_args() { return this->args; }

unsigned int BusCommandProxy::get_serial() { return this->serial; }

string BusCommandProxy::my_id() { return this->proxy_node->node_id(); }

string BusCommandProxy::peer_id() { return this->proxy_node->peer_id; }

void BusCommandProxy::raise_error(const string& error_message, unsigned int error_code) {
    string prefix = "Error ";
    if (error_code > 0) {
        prefix += std::to_string(error_code) + " ";
    }
    Utils::error(prefix + " on peer - " + error_message);
}

void BusCommandProxy::raise_error_on_peer(const string& error_message, unsigned int error_code) {
    to_remote_peer(PEER_ERROR, {error_message, std::to_string(error_code)});
}

// -------------------------------------------------------------------------------------------------
// ProxyNode API

void ProxyNode::remote_call(const string& command, const vector<string>& args) {
    LOG_DEBUG("Remote command: <" << command << "> arrived at ProxyNode " << this->node_id());
    this->proxy->from_remote_peer(command, args);
}

void ProxyNode::to_remote_peer(const string& command, const vector<string>& args) {   
    if (this->peer_id == "") {
        Utils::error("Unknown peer");
    }
    vector<string> new_args(args);
    new_args.push_back(command);
    send(PROXY_COMMAND, new_args, this->peer_id);
}

void ProxyNode::node_joined_network(const string& node_id) {
    StarNode::node_joined_network(node_id);
    this->peer_id = node_id;
}

bool ProxyNode::is_server() { return StarNode::is_server; }

// -------------------------------------------------------------------------------------------------
// Messages

shared_ptr<Message> ProxyNode::message_factory(string& command, vector<string>& args) {
    std::shared_ptr<Message> message = DistributedAlgorithmNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (command == ProxyNode::PROXY_COMMAND) {
        vector<string> new_args(args);
        new_args.pop_back();
        return std::shared_ptr<Message>(new ProxyMessage(args.back(), new_args));
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
