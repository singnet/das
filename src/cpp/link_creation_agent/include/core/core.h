/**
 * @file core.h
 * @brief Core class to handle link creation requests
 *
 * This file contains the definition of the LinkCreationAgent class and the
 * LinkCreationAgentRequest struct. The LinkCreationAgent class is responsible
 * for managing link creation requests, including retrieving requests from a
 * DAS Node server, sending query requests, processing iterators, and creating
 * links. It also handles loading and saving configurations and request buffers.
 */
#pragma once
#include <string>
#include <vector>
#include "remote_iterator.h"
#include "das_server_node.h"
#include "das_link_creation_node.h"
#include "das_query_node.h"
#include "service.h"

using namespace query_element;
using namespace std;

namespace link_creation_agent
{
    struct LinkCreationAgentRequest
    {
        string query;
        string link_template;
        int max_results;
        int repeat;
        long last_execution; // last time executed
        int interval;        // current interval
        bool infinite = false;
    };

    class LinkCreationAgent
    {

    public:
        LinkCreationAgent(
            string config_path,
            query_node::QueryNode *query_node_client,
            query_node::QueryNode *query_node_server,
            lca::LinkCreationNode *link_creation_server,
            das::ServerNode *das_client); // DASNode client, DASNode server

        ~LinkCreationAgent() {};

        /**
         * @brief Retrieve a request from DAS Node server or get a request from the requests buffer,
         * send a query request using DAS Node client, retrieve remote iterator and
         * send to LCAService to process the iterator and create links.
         */
        void run();

    private:
        /**
         * @brief Clean request's queue
         */
        void clean_requests();
        /**
         * @brief Sends a query to DAS Query Agent
         * @returns Returns a shared_ptr<RemoteIterator>, to iterate through the requests
         */
        shared_ptr<RemoteIterator> query();
        /**
         * @brief Load config file
         */
        void load_config();
        /**
         * @brief Save all requests that have the infinite value set as true to the disk or DB.
         */
        void save_buffer();
        void load_buffer();
        /**
         * @brief Handle the create link request.
         * @param request String received from a DAS Node server
         */
        void handleRequest(string request);

        vector<LinkCreationAgentRequest> request_buffer;
        string config_path;
        int default_interval; // Interval to run infinity tasks
        int thread_count;
        // service
        LinkCreationService service;
        static query_node::QueryNode query_node_client;
        static query_node::QueryNode query_node_server;
        static lca::LinkCreationNode link_creation_server;
        static das::ServerNode das_client;
    };
} // namespace link_creation_agent
