#include "DistributedAlgorithmNode.h"

#include "Utils.h"

using namespace distributed_algorithm_node;
using namespace commons;

DistributedAlgorithmNode::DistributedAlgorithmNode(const string& node_id,
                                                   LeadershipBrokerType leadership_algorithm,
                                                   MessageBrokerType messaging_backend) {
    this->my_node_id = node_id;
    this->leadership_broker = LeadershipBroker::factory(leadership_algorithm);
    this->message_broker =
        MessageBroker::factory(messaging_backend,
                               // Empty destructor to avoid node deletion
                               shared_ptr<MessageFactory>(this, [](MessageFactory*) {}),
                               node_id);
}

DistributedAlgorithmNode::~DistributedAlgorithmNode() { this->graceful_shutdown(); }

// -------------------------------------------------------------------------------------------------
// Public API

void DistributedAlgorithmNode::join_network() {
    this->leadership_broker->set_message_broker(this->message_broker);
    this->message_broker->join_network();
    // Utils::sleep(1000);
    string my_leadership_vote = this->cast_leadership_vote();
    this->leadership_broker->start_leader_election(my_leadership_vote);
    while (!this->has_leader()) {
        Utils::sleep();
    }
    vector<string> args;
    args.push_back(this->node_id());
    this->message_broker->broadcast(this->known_commands.NODE_JOINED_NETWORK, args);
}

bool DistributedAlgorithmNode::is_leader() { return (this->leader_id() == this->node_id()); }

string DistributedAlgorithmNode::leader_id() { return this->leadership_broker->leader_id(); }

bool DistributedAlgorithmNode::has_leader() { return this->leadership_broker->has_leader(); }

void DistributedAlgorithmNode::add_peer(const string& peer_id) {
    this->message_broker->add_peer(peer_id);
}

string DistributedAlgorithmNode::node_id() { return this->my_node_id; }

void DistributedAlgorithmNode::broadcast(const string& command, const vector<string>& args) {
    this->message_broker->broadcast(command, args);
}

void DistributedAlgorithmNode::send(const string& command,
                                    const vector<string>& args,
                                    const string& recipient) {
    this->message_broker->send(command, args, recipient);
}

std::shared_ptr<Message> DistributedAlgorithmNode::message_factory(string& command,
                                                                   vector<string>& args) {
    if (command == this->known_commands.NODE_JOINED_NETWORK) {
        return std::shared_ptr<Message>(new NodeJoinedNetwork(args[0]));
    } else {
        return std::shared_ptr<Message>{};
    }
}

void DistributedAlgorithmNode::graceful_shutdown() { this->message_broker->graceful_shutdown(); }

string DistributedAlgorithmNode::to_string() {
    string answer = "[node_id: " + this->node_id() + ", leader: " + leader_id() + ", peers: {";
    for (auto peer: this->message_broker->peers) {
        answer += peer;
        answer += ", ";
    }
    answer.pop_back();
    answer.pop_back();
    answer += "}]";
    return answer;
}
