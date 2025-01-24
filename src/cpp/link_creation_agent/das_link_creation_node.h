/**
 * @brief DAS Node server to receive link creation requests
 */
#pragma once
#include "queue.h"
#include "star_node.h"
using namespace atom_space_node;

namespace link_creation_agent {
class LinkCreationNode : public StarNode {
   public:
    /**
     * @brief Server constructor
     * @param node_id ID of this node in the network.
     */
    LinkCreationNode(const string& node_id);
    /**
     * Destructor
     */
    ~LinkCreationNode();
    /**
     * @brief Retrieves the next request
     */
    string pop_request();
    /**
     * @brief Return true if the request's queue is empty
     */
    bool is_query_empty();
    /**
     * @brief Return true if the server is shutting down
     */
    bool is_shutting_down();

    void add_request(string& request);

    string to_string() { return "LinkCreationNode"; }

    virtual shared_ptr<Message> message_factory(string& command, vector<string>& args);

   private:
    Queue<string> request_queue;
    const string CREATE_LINK = "create_link";
    bool shutting_down = false;
};

class LinkCreationRequest : public Message {
   public:
    string command;
    vector<string> args;
    LinkCreationRequest(string command, vector<string>& args);
    void act(shared_ptr<MessageFactory> node);
};
}  // namespace link_creation_agent
