#ifndef _DISTRIBUTED_ALGORITHM_NODE_DISTRIBUTEDALGORITHMNODE_H
#define _DISTRIBUTED_ALGORITHM_NODE_DISTRIBUTEDALGORITHMNODE_H

#include "LeadershipBroker.h"
#include "MessageBroker.h"
#include "Message.h"
#include <string>
#include <vector>

using namespace std;

namespace distributed_algorithm_node {

/**
 * Implements a node in a network used for some distributed processing.
 *
 * This is an abstract class so users of this module are supposed to extend it to
 * implement its own distributed network.
 *
 * Communication between nodes are performed by Message objects which are like
 * a piece of code sent from one node to another. We don't actually send
 * code but instead we send a reference to what type of Message object is being
 * exchanged so once the message arrive in the recipient a Message object is built
 * and its code is executed. The "reference" about what type of Message is being
 * exchanged is encoded like command lines with commands and its arguments.
 *
 * This is what you should care about when extending DistributedAlgorithmNode:
 *
 *   - DistributedAlgorithmNode is the class that build Message objects. That's why it extends
 *     MessageFactory. This abstract class already knows how to build some basic Message
 *     objects for some commands which are common to all the networks (e.g. joining
 *     of new nodes to the network) but subclasses of DistributedAlgorithmNode should re-implement
 *     message_factory() to build Message objects for its own distributed application.
 *   - Message commands are executed in separated threads. So if such commands updates some
 *     state variable inside DistributedAlgorithmNode or its subclasses, this state variable should
 *     be protected by mutual exclusion primitives.
 *   - DistributedAlgorithmNode's constructor expects allocated objects for MessageBroker and
 *     LeadershipBroker. These are abstract classes which may have many concrete
 *     implementations. When designing your distributed network you need to decide
 *     which messaging system is supposed to be used to actually make the communication
 *     bwtween nodes. You can use one of the concrete implementations we provide or you
 *     can provide your own implementation extending these abstract classes. Typically, users
 *     wouldn't need to implement its own messaging system but it may be the case for the
 *     leadership election algorithm as this is highly dependent on the network topology
 *     and expected behaviour.
 *   - DistributedAlgorithmNode has some pure virtual methods that need to be implemented. These are
 *     the methods called by basic Message objects to acomplish basic tasks like leadership
 *     election and notification of new nodes joning the network. The proper way to respond
 *     to these requests are delegated to concrete classes extending DistributedAlgorithmNode.
 */
class DistributedAlgorithmNode : public MessageFactory {

public:

    /**
     * Destructor.
     */
    virtual ~DistributedAlgorithmNode();

    /**
     * Basic constructor.
     *
     * @param node_id The ID of this node. Typically is a string to identify this node
     * in the MessageBroker.
     * @param leadership_algorithm The concrete class to be used as leadership broker.
     * @param messaging_backend The concrete class to be used as message broker.
     */
    DistributedAlgorithmNode(
        const string &node_id,
        LeadershipBrokerType leadership_algorithm,
        MessageBrokerType messaging_backend
    );

    // --------------------------------------------------------------------------------------------
    // Public API

    /**
     * Joins a network of similar nodes.
     */
    void join_network();

    /**
     * Returns true iff this node is the leader of the network.
     *
     * @return true iff this node is the leader of the network.
     */
    bool is_leader();

    /**
     * Returns the id of the network leader, or "" if no leader is known by this node.
     *
     * @return The id of the network leader, or "" if no leader is known by this node.
     */
    string leader_id();

    /**
     * Returns true iff the network leader is known.
     *
     * @return true iff the network leader is known.
     */
    bool has_leader();

    /**
     * Adds a node in the list of known peers.
     *
     * Note that this methods DOESN'T add the passed peer into the network. The peer is
     * supposed to be already in the network.
     *
     * @param peer_id The ID of the node being added as a peer.
     */
    void add_peer(const string &peer_id);

    /**
     * Returns this node's ID.
     *
     * @return this node's ID.
     */
    string node_id();

    /**
     * Broadcasts a command to all nodes in the network.
     *
     * All nodes in the network will be reached (not only the known peers) and the command
     * will be executed.
     *
     * @param command The command to be executed in the target nodes.
     * @param args Arguments for the command.
     */
    void broadcast(const string &command, const vector<string> &args);

    /**
     * Sends a command to the passed node.
     *
     * The target node is supposed to be a known peer. If not, an exception is thrown.
     *
     * @param command The command to be executed in the target nodes.
     * @param args Arguments for the command.
     * @recipient The target node for the command.
     */
    void send(const string &command, const vector<string> &args, const string &recipient);

    /**
     * Build the Message object which is supposed to execute the passed command.
     *
     * @param command The command to be remotely executed.
     * @param args Arguments for the command.
     * @return A Message object
     */
    virtual shared_ptr<Message> message_factory(string &command, vector<string> &args);

    /**
     * Gracefully shuts down threads or any other resources being used.
     */
    virtual void graceful_shutdown();

    // --------------------------------------------------------------------------------------------
    // API to be extended by concrete subclasses

    /**
     * This method is called whenever a new node joins the network.
     *
     * Every node that joins the network will broadcast a Message to everyone already joined.
     * However, it's up to each node to decide whether or not the newly joined node will be added
     * as a known peer.
     *
     * @param node_id The ID of the newly joined node.
     */
    virtual void node_joined_network(const string &node_id) = 0;

    /**
     * Casts a vote (which is the ID of the node being voted) in a leadership election.
     *
     * @return This node's vote (which is the ID of the node being voted) for leader.
     */
    virtual string cast_leadership_vote() = 0;

    /**
     * Non-optimized (to be used only for debug)
     */
    virtual string to_string();

private:

    string my_node_id;
    shared_ptr<LeadershipBroker> leadership_broker;
    shared_ptr<MessageBroker> message_broker;

    struct {
        string NODE_JOINED_NETWORK = "node_joined_network";
    } known_commands;
};

} // namespace distributed_algorithm_node

#endif // _DISTRIBUTED_ALGORITHM_NODE_DISTRIBUTEDALGORITHMNODE_H
