#pragma once

#include "StarNode.h"

using namespace std;
using namespace commons;
using namespace distributed_algorithm_node;

namespace service_bus {

class BusCommandProxy;

// -------------------------------------------------------------------------------------------------
// Used for the 1:1 RPC between caller and processor of a command.

//
// Client-server DistributedAlgorithmNode used to make RPC between the caller and the processor of
// a bus command.
//
// Callers are not aware of this node. They use the API in BusCommandProxy instead.
//
// A pair of client-server nodes is setup everytime a bus command is issued (and taken by a
// processor) by creating pair of ProxyNode (one in each end of the command). The proxy in caller's
// side is the server and the one in command processor's side is the client. These nodes carries
// command lines (command and its arguments) passed by BusCommandProxy's API as arguments of a
// command PROXY_COMMAND.
//
// At each side of the client-server communication, a port number is picked from a shared  pool of
// available port numbers and returned to the pool when no longer required (i.e. when the proxy
// is destroyed).
//
// ProxyMessage encodes and decodes PROXY_COMMAND by packing/unpacking command+arguments into/from
// the list of arguments of PROXY_COMMAND.
//

class ProxyNode : public StarNode {
    friend class BusCommandProxy;

   public:
    static string PROXY_COMMAND;

    ProxyNode(BusCommandProxy* proxy, const string& node_id);
    ProxyNode(BusCommandProxy* proxy, const string& node_id, const string& server_id);
    ~ProxyNode();

    virtual shared_ptr<Message> message_factory(string& command, vector<string>& args);
    void to_remote_peer(const string& command, const vector<string>& args);
    void node_joined_network(const string& node_id);

    void remote_call(const string& command, const vector<string>& args);

   private:
    BusCommandProxy* proxy;
    string peer_id;
};

class ProxyMessage : public Message {
   public:
    ProxyMessage(const string& command, const vector<string>& args);
    void act(shared_ptr<MessageFactory> node);

   private:
    string command;
    vector<string> args;
};

// -------------------------------------------------------------------------------------------------
// BusCommandProxy

/**
 * Proxy instantiated in both ends of a command (in the caller that issued the command and in the
 * respectivwe command processor) in order to allow RPC communication between the peers.
 *
 * Every command processor implementation should also contain a subclass of BusCommandProxy so
 * that callers can use it to issue the command and get its response as well as possibly interacting
 * with the command processor using RPC.
 *
 * RPC communication is made using to_remote_peer() and from_remote_peer(). Both methods expect
 * a command line (i.e. a command and its arguments). to_remote_peer() is implemented in this
 * superclass but from_remote_peer() is supposed to be implemented in the subclasses because each
 * subclasses knows which/how command should be implemented.
 */
class BusCommandProxy {
    friend class ServiceBus;

   public:
    // Commands allowed at the proxy level (caller <--> processor)
    static string PEER_ERROR;  // Raise an error in peer

    /**
     * Basic constructor.
     */
    BusCommandProxy();

    /**
     * Constructor.
     *
     * Note: the command is not actually issued just by calling this constructor. In order to
     * issue the command, the caller need to call ServiceBus::issue_bus_command(). Once
     * properly issued, this proxy can be used to get command response and communicate with
     * the remote peer.
     *
     * @param command Command to be issued
     * @param args Arguments for the passed command
     */
    BusCommandProxy(const string& command, const vector<string>& args);

    /**
     * Destructor.
     */
    virtual ~BusCommandProxy();

    /**
     * Returns the command being issued
     *
     * $return The command being issued
     */
    const string& get_command();

    /**
     * Returns the arguments for the command being issued.
     *
     * @return The arguments for the command being issued.
     */
    const vector<string>& get_args();

    /**
     * Returns the request serial number.
     *
     * @return The request serial number.
     */
    virtual unsigned int get_serial();

    /**
     * Returns the node id of this proxy.
     *
     * @return The node id of this proxy.
     */
    string my_id();

    /**
     * Returns the node id of the remote proxy linked to this one.
     *
     * @return The node id of the remote proxy linked to this one.
     */
    virtual string peer_id();

    /**
     * Piggyback method called when raise_error_on_peer() is called in peer's side.
     */
    virtual void raise_error(const string& error_message, unsigned int error_code = 0);

    // ---------------------------------------------------------------------------------------------
    // BusCommandProxy RPC

    /**
     * Send the passed command and arguments to the remote peer (another concrete BusCommandProxy
     * of the same subclass).
     *
     * One peer BusCommandProxy of the same subclass is instantiated at each end of a command
     * (one peer in the caller and another one in the command processor). These peers can make
     * RPC calls one to eachother using to_remote_peer().
     *
     * The peer will have its from_remote_peer() method called passing the same command and
     * arguments.
     *
     * @param command RPC command
     * @param args RPC command's arguments
     */
    void to_remote_peer(const string& command, const vector<string>& args);

    /**
     * Receive a command and its arguments passed by the remote peer.
     *
     * Concrete subclasses of BusCommandProxy need to implement this method.
     *
     * @param command RPC command
     * @param args RPC command's arguments
     * @return true iff the command is supposed to be processed in this
     * class (rather than in a subclass).
     */
    virtual bool from_remote_peer(const string& command, const vector<string>& args);

    /**
     * Pack specific command args (in concrete subclasses) into args vector
     */
    virtual void pack_command_line_args() = 0;

    /**
     * Raise an error on remote peer by calling raise_error() passing the error code and message.
     *
     * Subclasses may override raise_error() to catch errors and prevent the exception from
     * being actually raised.
     */
    void raise_error_on_peer(const string& error_message, unsigned int error_code = 0);

    string command;
    vector<string> args;

   private:
    void setup_proxy_node(const string& client_id = "", const string& server_id = "");

    string requestor_id;
    unsigned int serial;
    unsigned long proxy_port;
    ProxyNode* proxy_node;
};

}  // namespace service_bus
