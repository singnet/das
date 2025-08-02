#ifndef _DISTRIBUTED_ALGORITHM_NODE_MESSAGEBROKER_H
#define _DISTRIBUTED_ALGORITHM_NODE_MESSAGEBROKER_H

#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

// TODO: Once das-proto is updated, update atom_space_node to distributed_algorithm_node

#include "atom_space_node.grpc.pb.h"
// #include "distributed_algorithm_node.grpc.pb.h"

#include "Message.h"
#include "SharedQueue.h"
#include "Stoppable.h"
#include "StoppableThread.h"

using namespace std;
using namespace commons;

namespace distributed_algorithm_node {

enum class MessageBrokerType { RAM, GRPC };

class DistributedAlgorithmNode;

// -------------------------------------------------------------------------------------------------
// Abstract superclass

/**
 * Implements the communication layer used by nodes to exchange Messages.
 *
 * This is the abstract class defining the API used by DistributedAlgorithmNodes to exchange messages.
 * Users of the DistributedAlgorithmNode module aren't supposed to interact with MessageBroker directly.
 */
class MessageBroker : public Stoppable {
   public:
    /**
     * Factory method for concrete subclasses.
     *
     * @param instance_type Defines which subclass should be used to instantiate the MessageBroker
     * @param host_node The object responsible for building Message objects. Typically, it's The
     * node this MessageBroker belongs to.
     * @param node_id The ID of the DistributedAlgorithmNode this MessageBroker belongs to.
     * @return An instance of the selected MessageBroker subclass.
     */
    static shared_ptr<MessageBroker> factory(MessageBrokerType instance_type,
                                             shared_ptr<MessageFactory> host_node,
                                             const string& node_id);

    /**
     * Basic constructor
     *
     * @param host_node The object responsible for building Message objects. Typically, it's The
     * node this MessageBroker belongs to.
     * @param node_id The ID of the DistributedAlgorithmNode this MessageBroker belongs to.
     */
    MessageBroker(shared_ptr<MessageFactory> host_node, const string& node_id);

    /**
     * Destructor.
     */
    virtual ~MessageBroker();

    // ----------------------------------------------------------------
    // Public concrete API

    /**
     * Adds a new peer to the list of known nodes.
     *
     * @param peer_id The ID of the newly known peer.
     */
    virtual void add_peer(const string& peer_id);

    /**
     * Returns true iff the passed peer has been previously added
     *
     * @param peer_id Peer id being checked.
     * @return true iff the passed peer has been previously added
     */
    bool is_peer(const string& peer_id);

    /**
     * Gracefully shuts down threads or any other resources being used in communication.
     */
    virtual void stop();

    /**
     * Returns true iff this MessageBroker is shuting down.
     *
     * The idea is to allow concrete subclasses to know when a graceful shutdown has been requested
     * so threads or any other resources being used in communication can be stoped/released/etc.
     */
    bool stopped();

    // ----------------------------------------------------------------
    // Public abstract API

    /**
     * Inserts the host node into the network.
     */
    virtual void join_network() = 0;

    /**
     * Broadcasts a command to all nodes in the network.
     *
     * All nodes in the network will be reached (not only the known peers) and the command
     * will be executed.
     *
     * @param command The command to be executed in the target nodes.
     * @param args Arguments for the command.
     */
    virtual void broadcast(const string& command, const vector<string>& args) = 0;

    /**
     * Sends a command to the passed node.
     *
     * The target node is supposed to be a known peer. If not, an exception is thrown.
     *
     * @param command The command to be executed in the target nodes.
     * @param args Arguments for the command.
     * @recipient The target node for the command.
     */
    virtual void send(const string& command, const vector<string>& args, const string& recipient) = 0;

    shared_ptr<MessageFactory> host_node;
    unordered_set<string> peers;
    mutex peers_mutex;
    string node_id;
    bool stop_flag;
    mutex stop_flag_mutex;
    bool joined_network;
};

// -------------------------------------------------------------------------------------------------
// Concrete subclasses

/**
 * Concrete implementation of MessageBroker using shared queues in RAM to exchange Message among
 * nodes.
 *
 * Nodes are supposed to be running in the same runtime process.
 */
class SynchronousSharedRAM : public MessageBroker {
   public:
    /**
     * Basic constructor
     *
     * @param host_node The object responsible for building Message objects. Typically, it's The
     * node this MessageBroker belongs to.
     */
    SynchronousSharedRAM(shared_ptr<MessageFactory> host_node, const string& node_id);

    /**
     * Destructor.
     */
    ~SynchronousSharedRAM();

    // ----------------------------------------------------------------
    // Public MessageBroker abstract API

    /**
     * Inserts the host node into the network.
     *
     * Initialize incoming messages queue and starts a thread to process it
     */
    virtual void join_network();

    /**
     * Broadcasts a command to all nodes in the network.
     *
     * All nodes in the network will be reached (not only the known peers) and the command
     * will be executed. Basically the Message is sent to all known peers which, in their turns,
     * re-send it to their known peers until there's no other peer to spread it. The GRPC object
     * used to send the request contains a list of visited nodes so a request is never re-sent
     * to nodes that have already received it.
     *
     * @param command The command to be executed in the target nodes.
     * @param args Arguments for the command.
     */
    virtual void broadcast(const string& command, const vector<string>& args);

    /**
     * Sends a command to the passed node.
     *
     * The target node is supposed to be a known peer. If not, an exception is thrown.
     * Uses the client GRPC channel to send the command to the target.
     *
     * @param command The command to be executed in the target nodes.
     * @param args Arguments for the command.
     * @recipient The target node for the command.
     */
    virtual void send(const string& command, const vector<string>& args, const string& recipient);

    /**
     * Gracefully shuts down threads or any other resources being used in communication.
     */
    void stop();

   private:
    static unsigned int MESSAGE_THREAD_COUNT;
    static unordered_map<string, SharedQueue*> NODE_QUEUE;
    static mutex NODE_QUEUE_MUTEX;

    vector<shared_ptr<StoppableThread>> inbox_threads;
    SharedQueue incoming_messages;  // Thread safe container

    // Methods used to start threads
    void inbox_thread_method(shared_ptr<StoppableThread> monitor);
};

/**
 * Concrete implementation of MessageBroker using GRPC to exchange Message among nodes.
 *
 * Synchronous GRPS calls are used to send commands between nodes in the network. When joining
 * the network, each node initializes a request queue and a thread to listen to a PORT for
 * GRPC calls (this thread is what GRPC's documentation calls a GRPC Server). This way this node
 * becomes capable of answering GRPC requests.
 *
 * In addition to this, another queue is initialized for outgoing Messages and another thread is
 * started to observe this queue.
 *
 * A Client GRPC channel is created for each the newly inserted peer.
 *
 * So the SynchronousGRPC MessageBroker have:
 *
 *   - An incoming queue with waiting-to-be-processed incoming commands
 *   - A GRPC thread listening for the rpc command requests
 *   - N threads reading from this queue and processing the requested commands.
 *   - An outgoing queue with waiting-to-be-sent outgoing commands
 *   - N threads observing the outgoing queue and processing this queue to send requests to other
 *     nodes.
 *
 * When one of the methods to send messages is called (e.g. broadcast() or send()), the passed
 * command is enqueued in the outgoing queue and the methoid returns immetialely, meaning that that
 * is no guarantee that the Message have been received by the other node(s) when the method returns.
 * A thread will dequeue the request and use a previously created client GRPC channel to make the
 * GRPC rpc call, sending the command to the target node.
 *
 * In the target node, the GRPC server thread will get the requested command and enequeue it in the
 * incomming queue. Then a thread will dequeue this request and execute the requested command on
 * the target node.
 *
 * No rpc answer is used in these GRPC calls. So if a command expects an answer to return, this
 * answer is supposed to be implemented as a separate Message going back from the target node to
 * the node that originated the request.
 */
// TODO: Once das-proto is updated, update atom_space_node to distributed_algorithm_node
// class SynchronousGRPC : public MessageBroker, public dasproto::DistributedAlgorithmNode::Service {
class SynchronousGRPC : public MessageBroker, public dasproto::AtomSpaceNode::Service {
   public:
    /**
     * Basic constructor
     *
     * @param host_node The object responsible for building Message objects. Typically, it's The
     * node this MessageBroker belongs to.
     * @param node_id The ID of the DistributedAlgorithmNode this MessageBroker belongs to.
     */
    SynchronousGRPC(shared_ptr<MessageFactory> host_node, const string& node_id);

    /**
     * Destructor.
     */
    ~SynchronousGRPC();

    /**
     * Adds additional processing to superclass' add_peer() basically only adds the new peer id
     * into a container. Here, a GRPC channel is created and stored in the object for further use
     * when sending Messages.
     *
     * @param peer_id The ID of the newly known peer.
     */
    virtual void add_peer(const string& peer_id);

    // ----------------------------------------------------------------
    // Public MessageBroker abstract API

    /**
     * Inserts the host node into the network.
     *
     * Initialize incoming and outgoing queues and starts threads to process each of them.
     * Also initializes the GRPC Server thread to listen to the GRPC calls.
     */
    virtual void join_network();

    /**
     * Broadcasts a command to all nodes in the network.
     *
     * All nodes in the network will be reached (not only the known peers) and the command
     * will be executed. Basically the Message is sent to all known peers which, in their turns,
     * re-send it to their known peers until there's no other peer to spread it. The GRPC object
     * used to send the request contains a list of visited nodes so a request is never re-sent
     * to nodes that have already received it.
     *
     * @param command The command to be executed in the target nodes.
     * @param args Arguments for the command.
     */
    virtual void broadcast(const string& command, const vector<string>& args);

    /**
     * Sends a command to the passed node.
     *
     * The target node is supposed to be a known peer. If not, an exception is thrown.
     * Uses the client GRPC channel to send the command to the target.
     *
     * @param command The command to be executed in the target nodes.
     * @param args Arguments for the command.
     * @recipient The target node for the command.
     */
    virtual void send(const string& command, const vector<string>& args, const string& recipient);

    /**
     * Gracefully shuts down threads or any other resources being used in communication.
     */
    void stop();

    // ----------------------------------------------------------------
    // Public GRPC API

    /**
     * Empty command to check if the GRPC server is available.
     *
     * This is a standard rpc in DAS proto which all servers implement.
     */
    grpc::Status ping(grpc::ServerContext* grpc_context,
                      const dasproto::Empty* request,
                      dasproto::Ack* reply) override;

    /**
     * Delivers a Message to be remotely executed.
     */
    grpc::Status execute_message(grpc::ServerContext* grpc_context,
                                 const dasproto::MessageData* request,
                                 dasproto::Empty* reply) override;

   private:
    static unsigned int MESSAGE_THREAD_COUNT;
    static mutex GRPC_BUILDER_MUTEX;
    unique_ptr<grpc::Server> grpc_server;
    shared_ptr<StoppableThread> grpc_thread;
    vector<shared_ptr<StoppableThread>> inbox_threads;
    SharedQueue incoming_messages;  // Thread safe container
    SharedQueue outgoing_messages;  // Thread safe container

    bool grpc_server_started_flag;
    mutex grpc_server_started_flag_mutex;
    bool inbox_setup_finished_flag;
    mutex inbox_setup_finished_flag_mutex;
    void set_grpc_server_started();
    bool grpc_server_started();
    void set_inbox_setup_finished();
    bool inbox_setup_finished();

    // Methods used to start threads
    void grpc_thread_teardown(shared_ptr<StoppableThread> monitor);
    void grpc_thread_method(shared_ptr<StoppableThread> monitor);
    void inbox_thread_method(shared_ptr<StoppableThread> monitor);
};

// -------------------------------------------------------------------------------------------------
// Common utility classes

class CommandLinePackage {
   public:
    CommandLinePackage(const string& command, const vector<string>& args);
    ~CommandLinePackage();
    string command;
    vector<string> args;
    bool is_broadcast;
    unordered_set<string> visited;
};

}  // namespace distributed_algorithm_node

#endif  // _DISTRIBUTED_ALGORITHM_NODE_MESSAGEBROKER_H
