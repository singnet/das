#include "das_link_creation_node.h"

using namespace link_creation_agent;
using namespace std;
using namespace distributed_algorithm_node;

LinkCreationNode::LinkCreationNode(const string &node_id) : StarNode(node_id)
{

}


LinkCreationNode::~LinkCreationNode()
{
    shutting_down = true;
    DistributedAlgorithmNode::graceful_shutdown();
    
}

string LinkCreationNode::pop_request()
{
    return request_queue.dequeue();
}


bool LinkCreationNode::is_query_empty()
{
    return request_queue.empty();
}

bool LinkCreationNode::is_shutting_down()
{
    return shutting_down;
}

void LinkCreationNode::add_request(string &request)
{
    request_queue.enqueue(request);
}

shared_ptr<Message> LinkCreationNode::message_factory(string &command, vector<string> &args)
{
    cout << "LinkCreationNode::message_factory" << endl;
    cout << command << endl;
    shared_ptr<Message> message = StarNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (command == CREATE_LINK)
    {
        return make_shared<LinkCreationRequest>(command, args);
    }
    cout << "Command not recognized" << endl;
    return make_shared<DummyMessage>(command, args);
}


LinkCreationRequest::LinkCreationRequest(string command, vector<string> & args)
{
        this->command = command;
        this->args = args;
}

void LinkCreationRequest::act(shared_ptr<MessageFactory> node)
{
    auto link_node = dynamic_pointer_cast<LinkCreationNode>(node);
    string request;
    for (auto arg : this->args)
    {
        request += arg + " ";
    }
    link_node->add_request(request);
}