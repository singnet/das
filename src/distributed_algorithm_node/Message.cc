#include "Message.h"
#include "DistributedAlgorithmNode.h"

using namespace distributed_algorithm_node;

Message::Message() {
}

Message::~Message() {
}

// -------------------------------------------------------------------------------------------------
// Specialized Message subclasses

NodeJoinedNetwork::~NodeJoinedNetwork() {
}

NodeJoinedNetwork::NodeJoinedNetwork(string &node_id) {
    this->joining_node = node_id;
}

void NodeJoinedNetwork::act(shared_ptr<MessageFactory> node) {
    auto distributed_algorithm_node = dynamic_pointer_cast<DistributedAlgorithmNode>(node);
    distributed_algorithm_node->node_joined_network(this->joining_node);
}
