#pragma once

#include <mutex>
#include <set>
#include <string>

#include "BusCommandProcessor.h"
#include "BusCommandProxy.h"
#include "BusNode.h"
#include "PortPool.h"
#include "Utils.h"

using namespace std;
using namespace commons;
using namespace distributed_algorithm_node;

namespace service_bus {

/**
 * Singleton (supposed to be instantiated through ServiceBusSingleton) which provides
 * the API for many DAS services provided by its agents.
 *
 * Elements in the bus can be either command providers or command issuers (or both).
 *
 * Command issuers just use ServiceBus to issue commands. They don't provide any service
 * (i.e. They don't process any commands) themselves. Command providers listen to the bus
 * for commands they're responsible for and process such commands when other bus element
 * issues them. Command providers must explicitely register themselves as so in order
 * to receive commands from the bus.
 *
 * The list of commands is fixed (hard-wired in code) and represents the services provided
 * by agents in the DAS architecture. As agents join the bus, they register themselves as
 * command providers taking the ownership of commands in this list. Any attempt to take
 * ownership of a command that have already been taken will throw an exception. It means
 * that each service (i.e. each command) may be assigned to only one bus element.
 * Any attemps to issue a command which hasn't be assigned to any bus element will also
 * throw an exception.
 *
 * This is the list of commands available in the DAS service bus:
 * (TODO: update this list and add hints for commands' arguments)
 *
 * - PATTERN_MATCHING_QUERY
 * - QUERY_EVOLUTION
 *
 * Command processors must extend BusCommandProcessor and be registered by calling
 * register_processor(). Command issuers must create BusCommandProxy objects in order
 * to issue commands by calling issue_bus_command().
 *
 * There's one BusCommandProxy concrete subclass for each bus command. In addition to
 * the information of the command and its arguments, this proxy object is also aware of
 * the actual object that's delivered to the caller as command response (e.g
 * PATTERN_MATCHING_QUERY synchronously delivers an interator which is asynchronously feed
 * up by the corresponding command processor. In addition to this, this proxy also allow
 * RPC communication between the command caller and respective processor.
 */
class ServiceBus {
    // ---------------------------------------------------------------------------------------------
    // Private inner classes used for RPC among bus elements

   private:
    class Node : public BusNode {
       public:
        Node();
        Node(const string& id,
             shared_ptr<BusNode::Bus> bus,
             const set<string>& node_commands,
             const string& known_peer);

        shared_ptr<Message> message_factory(string& command, vector<string>& args);
        shared_ptr<BusCommandProcessor> processor;

       private:
        shared_ptr<BusNode::Bus> bus;
    };

    class BusCommandMessage : public Message {
       public:
        BusCommandMessage(const string& command, const vector<string>& args);
        void act(shared_ptr<MessageFactory> node);

       private:
        string command;
        vector<string> args;
    };

    // ---------------------------------------------------------------------------------------------
    // Private static state initialized by ServiceBusSingleton

    static set<string> SERVICE_LIST;

    // ---------------------------------------------------------------------------------------------
    // Private state

    shared_ptr<ServiceBus::Node> bus_node;
    shared_ptr<BusNode::Bus> bus;
    mutex api_mutex;
    unsigned int next_request_serial;

    // ---------------------------------------------------------------------------------------------
    // Public API

   public:
    static string PATTERN_MATCHING_QUERY;
    static string QUERY_EVOLUTION;
    static string LINK_CREATION;
    static string INFERENCE;

    /**
     * Registers a processor making it take the ownership of one or more bus commands.
     *
     * Note: the actual list of commands being taken is defined in BusCommandProcessor's constructor
     *
     * @param processor The BusCommandProcessor taking ownership of bus command(s).
     */
    virtual void register_processor(shared_ptr<BusCommandProcessor> processor);

    /**
     * Issues a command in the bus.
     *
     * If the passed command is not in the list of bus commands or if no processors have taken
     * ownership for it yet, an exception is thrown.
     *
     * @param bus_command A BusCommandProxy with the command and its arguments.
     */
    virtual void issue_bus_command(shared_ptr<BusCommandProxy> bus_command);

    // ---------------------------------------------------------------------------------------------
    // Used by ServiceBusSingleton

    /**
     * This method is not actually part of the API, it's supposed to be called
     * by ServiceBusSingleton. It's kept public to make it easier to write unit tests.
     */
    static void initialize_statics(const set<string>& commands = {},
                                   unsigned int port_lower = 64000,
                                   unsigned int port_upper = 64999);

    /**
     * Constructor is not actually part of the API, it's supposed to be called
     * by ServiceBusSingleton. It's kept public to make it easier to write unit tests.
     */
    ServiceBus(const string& host_id, const string& known_peer = "");

    /**
     * Destructor
     */
    ~ServiceBus();
};

}  // namespace service_bus
