/**
 * @file agent.h
 * @brief Agent class to handle link creation requests
 *
 * This file contains the definition of the LinkCreationAgent class and the
 * LinkCreationAgentRequest struct. The LinkCreationAgent class is responsible
 * for managing link creation requests, including retrieving requests from a
 * DAS Node server, sending query requests, and creating links. 
 * It also handles loading and saving configurations and request buffers.
 */
#pragma once
#include <string>
#include <vector>
#include "remote_iterator.h"
#include "das_server_node.h"
#include "das_link_creation_node.h"
#include "das_query_node_client.h"
#include "service.h"
#include <thread>

using namespace query_node;
using namespace std;

namespace link_creation_agent
{
    struct LinkCreationAgentRequest
    {
        string query;
        string link_template;
        int max_results = 1000;
        int repeat = 1;
        long last_execution; 
        int current_interval;        
        bool infinite = false;
    };

    class LinkCreationAgent
    {

    public:
        LinkCreationAgent(string config_path);
        ~LinkCreationAgent();

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
        /**
         * @brief Load all requests that have the infinite value set as true from the disk or DB.
         */
        void load_buffer();
        /**
         * @brief Handle the create link request.
         * @param request String received from a DAS Node server
         */
        void handleRequest(string request);

        void stop();

        // Attributes loaded from config file
        string config_path; // Path to the configuration file
        int default_interval; // Default interval to send requests
        int thread_count; // Number of threads to process requests
        string query_node_client_id; // ID of the query node client
        string query_node_server_id; // ID of the query node server
        string link_creation_server_id; // ID of the link creation server
        string das_client_id; // ID of the DAS client


        // Other attributes
        LinkCreationService *service;
        vector<LinkCreationAgentRequest> *request_buffer;
        query_node::QueryNode *query_node_client;
        LinkCreationNode *link_creation_node_server;
        das::ServerNode *das_client;
        thread *agent_thread;
        int loop_interval = 1;
    };
} // namespace link_creation_agent
