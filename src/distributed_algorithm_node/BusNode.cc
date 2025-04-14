#include "BusNode.h"

#include "LeadershipBroker.h"
#include "MessageBroker.h"
#include "Utils.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace distributed_algorithm_node;
using namespace std;

string BusNode::SET_COMMAND_OWNERSHIP = "set_command_ownership";

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

BusNode::BusNode(const string& node_id,
                 const Bus& bus,
                 const set<string>& node_commands,
                 const string& known_peer,
                 MessageBrokerType messaging_backend)
    : DistributedAlgorithmNode(node_id, LeadershipBrokerType::TRUSTED_BUS_PEER, messaging_backend) {
    this->bus = bus;
    this->trusted_known_peer_id = known_peer;
    this->my_commands = node_commands;
    if (known_peer != "") {
        LOG_DEBUG("New BUS node: " << node_id << " (known peer: " << known_peer << ")");
        this->is_master = false;
        this->add_peer(known_peer);
    } else {
        LOG_INFO("New BUS on: " << node_id);
        this->is_master = true;
    }
    this->join_network();
    this->join_bus();
}

// -------------------------------------------------------------------------------------------------
// DistributedAlgorithmNode virtual API

void BusNode::node_joined_network(const string& node_id) {
    if (this->is_master = true) {
        LOG_INFO("New element " << node_id << " joined the service BUS");
    }
    this->add_peer(node_id);
    broadcast_my_commands(node_id);
}

string BusNode::cast_leadership_vote() {
    if (this->is_master) {
        return this->node_id();
    } else {
        return this->trusted_known_peer_id;
    }
}

shared_ptr<Message> BusNode::message_factory(string& command, vector<string>& args) {
    shared_ptr<Message> message = DistributedAlgorithmNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (command == BusNode::SET_COMMAND_OWNERSHIP) {
        unsigned int n = args.size();
        if (n == 0) {
            Utils::error("Invalid command args: " + command);
        }
        vector<string> command_list;
        for (unsigned int i = 1; i < n; i++) {
            command_list.push_back(args[i]);
        }
        return shared_ptr<Message>(new SetCommandOwnership(args[0], command_list));
    }
    return shared_ptr<Message>{};
}

// -------------------------------------------------------------------------------------------------
// BusNode public methods

void BusNode::set_ownership(const string& command, const string& node_id) {
    this->bus.set_ownership(command, node_id);
}

const string& BusNode::get_ownership(const string& command) { return this->bus.get_ownership(command); }

void BusNode::send_bus_command(const string& command, const vector<string>& args) {
    string target_id = this->bus.get_ownership(command);
    if (target_id == "") {
        Utils::error("Bus: no owner is defined for command <" + command + ">");
    } else {
        LOG_DEBUG("BUS node " << this->node_id() << " is routing command " << command << " to " << target_id);
        send(command, args, target_id);
    }
}

void BusNode::take_ownership(const set<string>& commands) {
    for (auto command : commands) {
        LOG_DEBUG("BUS node " << this->node_id() << " is taking ownership of command " << command);
        this->bus.set_ownership(command, node_id());
        this->my_commands.insert(command);
    }
    broadcast_my_commands();
}

string BusNode::to_string() {
    string answer = DistributedAlgorithmNode::to_string();
    answer += " Bus: " + this->bus.to_string();
    return answer;
}

// -------------------------------------------------------------------------------------------------
// BusNode private methods

void BusNode::join_bus() { broadcast_my_commands(); }

void BusNode::broadcast_my_commands(const string& target_id) {
    if (this->my_commands.size() > 0) {
        vector<string> args;
        args.push_back(this->node_id());
        for (auto command : this->my_commands) {
            this->set_ownership(command, this->node_id());
            args.push_back(command);
        }
        if (target_id == "") {
            broadcast(BusNode::SET_COMMAND_OWNERSHIP, args);
        } else {
            send(BusNode::SET_COMMAND_OWNERSHIP, args, target_id);
        }
    }
}

// -------------------------------------------------------------------------------------------------
// Bus methods

BusNode::Bus::Bus() {}

BusNode::Bus::Bus(const Bus& other) { this->command_owner = other.command_owner; }

bool BusNode::Bus::operator==(const Bus& other) { return this->command_owner == other.command_owner; }

BusNode::Bus& BusNode::Bus::operator=(const Bus& other) {
    Bus aux(other);
    this->command_owner = aux.command_owner;
    return *this;
}

BusNode::Bus& BusNode::Bus::operator+(const string& command) {
    this->add(command);
    return *this;
}

void BusNode::Bus::add(const string& command) {
    if (this->command_owner.find(command) != this->command_owner.end()) {
        if (this->command_owner[command] != "") {
            Utils::error("Bus: command <" + command + "> " + "is already assigned to " +
                         this->command_owner[command]);
        }
    } else {
        this->command_owner[command] = "";
    }
}

void BusNode::Bus::set_ownership(const string& command, const string& node_id) {
    if (this->command_owner.find(command) == this->command_owner.end()) {
        Utils::error("Bus: command <" + command + "> " + "is not defined");
    } else {
        if (this->command_owner[command] == "") {
            this->command_owner[command] = node_id;
        } else {
            if (this->command_owner[command] != node_id) {
                Utils::error("Bus: command <" + command + "> " + "is already assigned to " +
                             this->command_owner[command]);
            }
        }
    }
}

const string& BusNode::Bus::get_ownership(const string& command) {
    auto pair = this->command_owner.find(command);
    if (pair == this->command_owner.end()) {
        Utils::error("Bus: unknown command <" + command + ">");
    }
    return pair->second;
}

bool BusNode::Bus::contains(const string& command) {
    return (this->command_owner.find(command) != this->command_owner.end());
}

string BusNode::Bus::to_string() {
    string answer = "{";
    if (this->command_owner.size() > 0) {
        for (auto pair : this->command_owner) {
            if (pair.second == "") {
                answer += pair.first + ", ";
            } else {
                answer += pair.first + ":" + pair.second + ", ";
            }
        }
        answer.pop_back();
        answer.pop_back();
    }
    answer += "}";
    return answer;
}

// -------------------------------------------------------------------------------------------------
// Messages

SetCommandOwnership::SetCommandOwnership(const string& node_id, const vector<string>& command_list) {
    this->node_id = node_id;
    this->command_list = command_list;
}

void SetCommandOwnership::act(shared_ptr<MessageFactory> node) {
    auto bus_node = dynamic_pointer_cast<BusNode>(node);
    bus_node->add_peer(this->node_id);
    for (auto command : this->command_list) {
        bus_node->set_ownership(command, this->node_id);
    }
}
