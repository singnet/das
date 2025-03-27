#ifndef _SERVICE_BUS_SERVICEBUS_H
#define _SERVICE_BUS_SERVICEBUS_H

#include <set>
#include <string>

#include "BusCommandProcessor.h"
#include "BusNode.h"

using namespace std;
using namespace distributed_algorithm_node;

namespace service_bus {

/**
 * Singleton (supposed to be instantiated through ServiceBusSingleton) which provides
 * the API for many DAS services provided by its agents.
 *
 * Elements in the bus can be either command providers or command issuers (or both).
 *
 * Command issuers just use ServiceBus to issue commands. It doesn't provide any service
 * (i.e. it doesn't process any commands) itself. Command providers listen to the bus
 * for commands it's responsible for and process such commands when other bus element
 * issues them. Command providers must explicitely register themselves as so in order
 * to receive commands.
 *
 * The list of commands is fixed and represents the services provided by agents in
 * the DAS architecture. As agents join the bus, they register themselves as command
 * providers taking the ownership of commands in this list. Any attempt to take
 * ownership of a command that have already been taken will throw an exception. It means
 * that each service (i.e. each command) may be assigned to only one bus element.
 * Any attemps to issue a command which hasn't be assigned to any bus element will also
 * throw an exception.
 *
 * This is the list of commands available in the DAS service bus:
 * (TODO: update this list and add hints for commands' arguments)
 *
 * - PATTERN_MATCHING_QUERY
 */
class ServiceBus {
   private:
    class Node : public BusNode {
       public:
        Node(const string& id,
             shared_ptr<BusNode::Bus> bus,
             const set<string>& node_commands,
             const string& known_peer);

        shared_ptr<Message> message_factory(string& command, vector<string>& args);
        shared_ptr<BusCommandProcessor> processor;

       private:
        shared_ptr<BusNode::Bus> bus;
    };

    class BusCommand : public Message {
       public:
        BusCommand(const string& command, const vector<string>& args);
        void act(shared_ptr<MessageFactory> node);

       private:
        string command;
        vector<string> args;
    };

    static set<string> SERVICE_LIST;
    shared_ptr<ServiceBus::Node> bus_node;
    shared_ptr<BusNode::Bus> bus;

   public:
    // ---------------------------------------------------------------------------------------------
    // Public API

    /**
     * Registers a processor making it take the ownership of one or more bus commands.
     *
     * Note: the actual list of commands being taken is defined in BusCommandProcessor's constructor
     *
     * @param processor The BusCommandProcessor taking ownership of bus command(s).
     */
    void register_processor(shared_ptr<BusCommandProcessor> processor);

    // ---------------------------------------------------------------------------------------------
    // Used by ServiceBusSingleton

    /**
     * This method is not actually part of the API, it's supposed to be called
     * by ServiceBusSingleton. It's kept public to make it easier to write unit tests.
     */
    static void initialize_statics(const set<string>& commands = {}) {
        if (commands.size() > 0) {
            for (auto command : commands) {
                SERVICE_LIST.insert(command);
            }
        } else {
            SERVICE_LIST.insert("PATTERN_MATCHING_QUERY");
        }
    }

    /**
     * Constructor is not actually part of the API, it's supposed to be called
     * by ServiceBusSingleton. It's kept public to make it easier to write unit tests.
     */
    ServiceBus(const string& host_id, const string& known_peer = "");
};

}  // namespace service_bus

#endif  // _SERVICE_BUS_SERVICEBUS_H
