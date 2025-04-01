#pragma once

#include "StarNode.h"

using namespace std;
using namespace distributed_algorithm_node;

namespace service_bus {

/**
 *
 */
class ProxyNode : public StarNode {
   public:
    static string PROXY_COMMAND;

    ProxyNode(const string& node_id);
    ProxyNode(const string& node_id, const string& server_id);
    ~ProxyNode();

    virtual shared_ptr<Message> message_factory(string& command, vector<string>& args);
    void remote_call(string& command, vector<string>& args);
};

class ProxyMessage : public Message {
   public:
    ProxyMessage(const string& command, const vector<string>& args);
    void act(shared_ptr<MessageFactory> node);
   private:
    string command;
    vector<string> args;
};

} // namespace service_bus
