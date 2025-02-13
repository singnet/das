#ifndef _DISTRIBUTED_ALGORITHM_NODE_MESSAGE_H
#define _DISTRIBUTED_ALGORITHM_NODE_MESSAGE_H

#include <string>
#include <vector>
#include <memory>

using namespace std;

namespace distributed_algorithm_node {

class DistributedAlgorithmNode;
class Message;

/**
 * Interface to be implemented by nodes (concrete implementations of DistributedAlgorithmNode) in order to
 * provide a factory method for the types of messages defined in its specific network.
 */
class MessageFactory {

public:

    /**
     * Message factory method.
     *
     * @param command The command to be executed in the target nodes.
     * @param args Arguments for the command.
     * @return An object of the proper class to deal with the passed command.
     */
    virtual shared_ptr<Message> message_factory(string &command, vector<string> &args) = 0;
};

// -------------------------------------------------------------------------------------------------
// Abstract superclass

/**
 * Basic abstract class for Messages to be exchanged among nodes in the network.
 *
 * Concrete subclasses should implement the act() method to perform whatever operation is
 * required in the recipient node.
 *
 * Note that Message objects don't actually get serialized and sent through the network.
 * What's actually sent is some identifier that makes possible for the recipients to
 * know which concrete class of Message should be instantiated there.
 *
 * Once the recipient receives this identifier, it instantiates an object of the proper
 * class (a concrete implementation of Message) and executed act(), passing the target
 * node as parameter.
 */
class Message {

public:

    /**
     * Executes the action defined in the Message in the recipient node, which is passed as
     * parameter.
     *
     * @param node The DistributedAlgorithmNode which received the Message.
     */
    virtual void act(shared_ptr<MessageFactory> node) = 0;

    /**
     * Empty constructor.
     */
    Message();

    /**
     * Destructor
     */
    ~Message();

private:

};

// -------------------------------------------------------------------------------------------------
// Concrete Messages used in basic DistributedAlgorithmNode settings

/**
 * Concrete Message implementation to deal with command "node_join_network", used
 * by DistributedAlgorithmNode to notify other nodes in the network of the presence of a newly
 * joined node.
 */
class NodeJoinedNetwork : public Message {

private:

    string joining_node;

public:

    /**
     * Basic constructor.
     *
     * @param node_id ID of the newly joined node.
     */
    NodeJoinedNetwork(string &node_id);

    /**
     * Destructor.
     */
    ~NodeJoinedNetwork();

    /**
     * Calls node->node_joined_neytwork() in the recipient node.
     */
    void act(shared_ptr<MessageFactory> node);
};

} // namespace distributed_algorithm_node

#endif // _DISTRIBUTED_ALGORITHM_NODE_MESSAGE_H
