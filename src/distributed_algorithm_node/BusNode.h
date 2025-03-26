#ifndef _DISTRIBUTED_ALGORITHM_NODE_STARNODE_H
#define _DISTRIBUTED_ALGORITHM_NODE_STARNODE_H

#include <string>

#include "DistributedAlgorithmNode.h"

using namespace std;

namespace distributed_algorithm_node {

/**
 * Node in a "fully-connected" topology used to implement a service bus.
 *
 * All nodes know a list of offered "services" and each node knows which services it's
 * able to provide. There's only 1 node enabled to provide each service.
 *
 */
class BusNode : public DistributedAlgorithmNode {
   public:
    /**
     * Inner class of BusNode. A wrapper for a container of strings that define a BUS in terms of the
     * services provided by the nodes in the BUS.
     *
     * A BUS is defined as the set of commands which can be issued by nodes in the BUS and its respective
     * node_ids of nodes respopnsible for executing each command. So, for instance, if the BUS is
     * supposed to have a agent A that processes commands C1 and C2 and another agent B that processes
     * command C3, then the BUS is defined as the mapping {C1->A, C2,->A C3->B}.
     */
    class Bus {
       public:
        /**
         * Empty constructor
         */
        Bus();

        /**
         * Copy constructor
         */
        Bus(const Bus& other);

        /**
         * Destructor
         */
        ~Bus();

        /**
         * Adds a command to this Bus.
         *
         * @param command Command to be added.
         */
        void add(const string& command);

        /**
         * Sets ownership of a command.
         *
         * The command should have no ownership or, if it does, this ownership should be set
         * for the same node_id otherwise an exception is thrown.
         *
         * @param command Command to set ownership.
         * @param node_id ID of the BusNode responsible for the passed command.
         */
        void set_ownership(const string& command, const string& node_id);

        /**
         * Get the node_id of the BusNode with ownership of passed command.
         *
         * @param command Command being looked up
         * @return The node_id of the BusNode with command's ownership or "" is none is set
         */
        const string& get_ownership(const string& command);

        /**
         * Returns a string representation of this class (mainly for debugging; not optimized to
         * production environment).
         */
        string to_string();

        bool operator==(const Bus& other);
        Bus& operator=(const Bus& other);
        Bus& operator+(const string& command);

       private:
        map<string, string> command_owner;

    };  // Inner class Bus

    // --------------------------------------------------------------------------------------------
    // Message commands

    static string SET_COMMAND_OWNERSHIP;

    // --------------------------------------------------------------------------------------------
    // Constructors and destructors

    /**
     * Server constructor.
     *
     * @param bus_id ID of the bus this node is joining in.
     * @param known_peer ID of a peer known to be in the bus. If not passed (or if "" is passed),
     * this is the first node in the bus.
     */
    BusNode(const string& node_id,
            const Bus& bus,
            const set<string>& node_commands,
            const string& known_peer = "",
            MessageBrokerType messaging_backend = MessageBrokerType::GRPC);

    /**
     * Destructor
     */
    virtual ~BusNode();

    // --------------------------------------------------------------------------------------------
    // DistributedAlgorithmNode virtual API

    /**
     * Method called when a new node is inserted in the network after this one has already joined.
     * All nodes keep track of every other nodes in the network.
     *
     * @param node_id ID of the newly inserted node.
     */
    virtual void node_joined_network(const string& node_id);

    /**
     * Method called when a leadershipo election is requested.
     *
     * If the node is the first node in the bus ("" or none is passed as known_peer in the
     * constructor), it votes in itself. Otherwise, it votes in its known_peer.
     */
    string cast_leadership_vote();

    /**
     * Build Message objects defined in BusNode.
     */
    virtual shared_ptr<Message> message_factory(string& command, vector<string>& args);

    // --------------------------------------------------------------------------------------------
    // BusNode public API

    /**
     * Set ownership of bus command.
     *
     * If command is unknown or if the passed bus_node_id is different from a previously made
     * setting, an exception is thrown.
     */
    void set_ownership(const string& command, const string& bus_node_id);

    /**
     * Get the node_id of the BusNode with ownership of passed command.
     *
     * @param command Command being looked up
     * @return The node_id of the BusNode with command's ownership or "" is none is set
     */
    const string& get_ownership(const string& command);

    /**
     * Sends a command to the node with ownership of the passed command.
     *
     * If no node has ownership of the command, an exception is thrown.
     *
     * @param command The command to be executed in the target node.
     * @param args Arguments for the command.
     */
    void send_bus_command(const string& command, const vector<string>& args);

    /**
     * Returns a string representation of this Node (mainly for debugging; not optimized to
     * production environment).
     */
    virtual string to_string();

   protected:
    bool is_master;
    string trusted_known_peer_id;
    Bus bus;

   private:
    set<string> my_commands;

    void join_bus();
    void broadcast_my_commands(const string& target_id = "");
};

/**
 * Concrete Message to set command ownership.
 */
class SetCommandOwnership : public Message {
   public:
    SetCommandOwnership(const string& node_id, const vector<string>& command_list);
    void act(shared_ptr<MessageFactory> node);

   private:
    string node_id;
    vector<string> command_list;
};

}  // namespace distributed_algorithm_node

#endif  // _DISTRIBUTED_ALGORITHM_NODE_STARNODE_H
