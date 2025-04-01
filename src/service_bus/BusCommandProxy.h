#pragma once

#include "SharedQueue.h"
#include "StarNode.h"

using namespace std;
using namespace commons;
using namespace distributed_algorithm_node;

namespace service_bus {

/**
 *
 */
class BusCommandProxy;
class ProxyNode : public StarNode {
    friend class BusCommandProxy;
   public:
    static string PROXY_COMMAND;

    ProxyNode(BusCommandProxy *proxy, const string& node_id);
    ProxyNode(BusCommandProxy *proxy, const string& node_id, const string& server_id);
    ~ProxyNode();

    virtual shared_ptr<Message> message_factory(string& command, vector<string>& args);
    void to_remote_peer(const string& command, const vector<string>& args);
    void node_joined_network(const string& node_id);

    void remote_call(const string &command, const vector<string> &args);
   private:
    BusCommandProxy *proxy;
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

/**
 *
 */
class BusCommandProxy  {
    friend class ServiceBus;

public:

    BusCommandProxy();
    BusCommandProxy(const string& command, const vector<string>& args);
    virtual ~BusCommandProxy();
    void setup_proxy_node(const string& client_id = "", const string &server_id = "");
    void to_remote_peer(const string& command, const vector<string>& args);
    virtual void from_remote_peer(const string& command, const vector<string>& args) = 0;

    string command;
    vector<string> args;

private:

    string requestor_id;
    unsigned int serial;
    unsigned long proxy_port;
    ProxyNode *proxy_node;
    SharedQueue *port_pool;
};

} // namespace service_bus
