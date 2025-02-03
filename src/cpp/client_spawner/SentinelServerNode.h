/**
 * @file sentinel_server_node.h
 * @brief Responsible for managing workers and handling their messages
*/
#pragma once

#include "StarNode.h"

using namespace distributed_algorithm_node;


namespace client_spawner {

constexpr const char* WORKER_NOTIFICATION = "worker_notification";

class SentinelServerNode: public StarNode {

public:
    SentinelServerNode(const string &node_id);

    ~SentinelServerNode();

    void storage_message(const string &worker_id, const string &message);
    
    virtual shared_ptr<Message> message_factory(string &command, vector<string> &args);
};

class WorkerMessage: public Message {

public:
    string worker_id;
    string msg;
    WorkerMessage(const string &worker_id, const string &msg);
    void act(shared_ptr<MessageFactory> node);

};

}  // namespace client_spawner

