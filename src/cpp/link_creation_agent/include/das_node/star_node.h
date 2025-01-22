#pragma once
#include <string>
#include <vector>
#include "DistributedAlgorithmNode.h"
#include <memory>

using namespace std;
using namespace distributed_algorithm_node;

namespace atom_space_node {

/**
 * Node in a "star" topology with one single server (which knows every other nodes in the network)
 * and N nodes (which know only the server).
 *
 * Use the different constructors to choose from client or server.
 */
class StarNode : public DistributedAlgorithmNode {

public:

    // --------------------------------------------------------------------------------------------
    // Constructors and destructors

    /**
     * Server constructor.
     *
     * @param node_id ID of this node in the network.
     * @param messaging_backend Type of network communication which will be used by the nodes.
     * in the network to exchange messages. Defaulted to GRPC.
     */
    StarNode(
        const string &node_id,
        MessageBrokerType messaging_backend = MessageBrokerType::GRPC
        );

    /**
     * Client constructor.
     *
     * @param node_id ID of this node in the network.
     * @param server_id ID of the server node.
     * @param messaging_backend Type of network communication which will be used by the nodes
     *        in the network to exchange messages. Defaulted to GRPC.
     */
    StarNode(
        const string &node_id, 
        const string &server_id,
        MessageBrokerType messaging_backend = MessageBrokerType::GRPC
        );

    /**
     * Destructor
     */
    virtual ~StarNode();

    // --------------------------------------------------------------------------------------------
    // AtomSpaceNode virtual API

    /**
     * Method called when a new node is inserted in the network after this one has already joined.
     * Server nodes will keep track of all newly inserted nodes. Client nodes disregard the info.
     *
     * @param node_id ID of the newly inserted node.
     */
    void node_joined_network(const string &node_id);

    /**
     * Method called when a leadershipo election is requested.
     *
     * Server nodes votes in themselves for leader while client node votes in their server.
     */
    string cast_leadership_vote();


    /**
     * Method to handle the message flow.
     */
    virtual shared_ptr<Message> message_factory(string &command, vector<string> &args);


protected:

    bool is_server;
    string server_id;
};

} // namespace atom_space_node
