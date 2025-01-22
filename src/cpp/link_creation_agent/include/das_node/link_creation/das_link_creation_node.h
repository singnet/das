/**
 * @brief DAS Node server to receive link creation requests
 */
#pragma once
// #include "DistributedAlgorithmNode.h"
#include "das_node/star_node.h"
#include "utils/queue.h"
using namespace atom_space_node;

namespace lca
{
    class LinkCreationNode : public StarNode
    {
    public:
        /**
         * @brief Server constructor
         * @param node_id ID of this node in the network.
         */
        LinkCreationNode(const string &node_id);
        /**
         * Destructor
         */
        ~LinkCreationNode();
        /**
         * @brief Retrieves the next request
         */
        string * pop_request();
        /**
         * @brief Return true if the request's queue is empty
         */
        bool is_query_empty();
        /**
         * @brief Return true if the server is shutting down
         */
        bool is_shutting_down();
    private:
        Queue<string*> request_queue;
        const string CREATE_LINK = "create_link";
    };

    class LinkCreationRequest : public Message
    {
        LinkCreationRequest(string command, vector<string> & args);
        void act(shared_ptr<MessageFactory> node);
    };
}
