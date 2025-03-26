#ifndef _SERVICE_BUS_SERVICEBUS_H
#define _SERVICE_BUS_SERVICEBUS_H

#include <set>
#include <string>
#include "BusNode.h"

using namespace std;
using namespace distributed_algorithm_node;

namespace service_bus {

/**
 *
 */
class ServiceBus {

public:

    class Node : public BusNode {

       public:

        Node(const string& id,
             const BusNode::Bus& bus,
             const set<string>& node_commands,
             const string& known_peer)
            : BusNode(id, bus, node_commands, known_peer, MessageBrokerType::RAM);

        virtual ~Node();

        shared_ptr<Message> message_factory(string& command, vector<string>& args);

       private:

        BusNode::Bus bus;
    };

    class BusCommand : public Message {

       public:

        BusCommand(const string& command, const vector<string>& args);
        void act(shared_ptr<MessageFactory> node);

       private:

        string command;
        vector<string> args;
    }

    ServiceBus();
    ~ServiceBus();

    static set<string> SERVICE_LIST;
    static void initialize_statics() {
        SERVICE_LIST.insert("PATTERN_MATCHING_QUERY");
    }

private:

    
};

} // namespace service_bus

#endif // _SERVICE_BUS_SERVICEBUS_H
