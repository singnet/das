#include "SentinelServerNode.h"

using namespace client_spawner;

SentinelServerNode::SentinelServerNode(const string &node_id) : StarNode(node_id) {}

SentinelServerNode::~SentinelServerNode() {}

// TODO: At this point Node does nothing with the messages.
void SentinelServerNode::storage_message(string &worker_id, string &message) {
    cout << worker_id <<" message received" << endl;
}

shared_ptr<Message> SentinelServerNode::message_factory(string &command, vector<string> &args) {
    shared_ptr<Message> message = StarNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (command == WORKER_NOTIFICATION) {
        return shared_ptr<Message>(new WorkerMessage(args[0], args[1]));
    }
    return shared_ptr<Message>{};
}


// =================================

WorkerMessage::WorkerMessage(string &worker_id, string &msg) {
    this->worker_id = worker_id;
    this->msg = msg;
}

void WorkerMessage::act(shared_ptr<MessageFactory> node) {
#ifdef DEBUG
    cout << "WorkerMessage::act() BEGIN" << endl;
    cout << "worker_id: " << this->worker_id << endl;
#endif
    auto sentinel_server_node = dynamic_pointer_cast<SentinelServerNode>(node);
    sentinel_server_node->storage_message(this->worker_id, this->msg);
#ifdef DEBUG
    cout << "WorkerMessage::act() END" << endl;
#endif
}
