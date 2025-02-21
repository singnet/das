/**
 * @file das_link_creation_node.h
 * @brief DAS Node server to receive link creation requests
 */
#pragma once
#include "StarNode.h"
#include "queue.h"
using namespace distributed_algorithm_node;

namespace link_creation_agent {
class LinkCreationNode : public StarNode {
   public:
    /**
     * @brief Server constructor
     * @param node_id ID of this node in the network.
     */
    LinkCreationNode(const string& node_id);
    /**
     * @brief Client constructor
     * @param node_id ID of this node in the network.
     * @param server_id ID of a server.
     */
    LinkCreationNode(const string& node_id, const string& server_id);;

    /**
     * Destructor
     */
    ~LinkCreationNode();
    /**
     * @brief Retrieves the next request
     */
    vector<string> pop_request();
    /**
     * @brief Return true if the request's queue is empty
     */
    bool is_query_empty();
    /**
     * @brief Return true if the server is shutting down
     */
    bool is_shutting_down();

    /**
     * @brief Add a request to the request queue
     * @param request Request to be added
     */
    void add_request(vector<string> request);

    /**
     * @brief Factory method to create a message
     * @param command Command to be executed
     * @param args Arguments to be passed to the command
     */
    virtual shared_ptr<Message> message_factory(string& command, vector<string>& args);

    void send_message(vector<string> args);

   private:
    Queue<vector<string>> shared_queue;
    const string CREATE_LINK = "create_link";  // DAS Node command
    const string CREATE_LINK_PROCESSOR = "create_link_processor"; 
    bool shutting_down = false;
    bool is_server = true;
};

/**
 * @brief Link creation request message
 */
class LinkCreationRequest : public Message {
   public:
    string command;
    vector<string> args;
    LinkCreationRequest(string command, vector<string>& args);
    void act(shared_ptr<MessageFactory> node);
    string server_id;
    string client_id;
};

/**
 * @brief Dummy message for unknown commands
 */
class DummyMessage : public Message {
   public:
    string command;
    vector<string> args;
    DummyMessage(string command, vector<string>& args) {
        this->command = command;
        this->args = args;
    }

    void act(shared_ptr<MessageFactory> node) {
        cout << "DummyMessage::act" << endl;
        cout << command << endl;
        for (auto arg : args) {
            cout << arg << endl;
        }
    }
};
}  // namespace link_creation_agent
