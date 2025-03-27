#ifndef _SERVICE_BUS_SERVICEBUS_H
#define _SERVICE_BUS_SERVICEBUS_H

#include <set>
#include <string>
#include "BusNode.h"
#include "BusCommandProcessor.h"

using namespace std;
using namespace distributed_algorithm_node;

namespace service_bus {

/**
 *
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

    static void initialize_statics(const set<string>& commands = {}) {
        if (commands.size() > 0) {
            for (auto command : commands) {
                SERVICE_LIST.insert(command);
            }
        } else {
            SERVICE_LIST.insert("PATTERN_MATCHING_QUERY");
        }
    }

    ServiceBus(const string& host_id, const string& known_peer = "");
    void register_processor(shared_ptr<BusCommandProcessor> processor);
};

} // namespace service_bus

#endif // _SERVICE_BUS_SERVICEBUS_H
