/**
 * @file LinkCreationAgent.h
 * @brief Agent class to handle link creation requests
 *
 * This file contains the definition of the LinkCreationAgent class and the
 * LinkCreationAgentRequest struct. The LinkCreationAgent class is responsible
 * for managing link creation requests, including retrieving requests from a
 * DAS Node server, sending query requests, and creating links.
 * It also handles loading and saving configurations and request buffers.
 */
#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "DASAgentNode.h"
#include "LinkCreationAgentNode.h"
#include "LinkCreationService.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBusSingleton.h"

using namespace std;
using namespace das_agent;
using namespace query_engine;
namespace link_creation_agent {
struct LinkCreationAgentRequest {
    vector<string> query;
    vector<string> link_template;
    int max_results = 1000;
    int repeat = 1;
    long last_execution = 0;
    int current_interval;
    bool infinite = false;
    string context = "";
    bool update_attention_broker = false;
    string id = "";
};

/**
 * @class LinkCreationAgent
 * @brief Manages the creation of links by processing requests from the DAS Node server or buffer.
 *
 * This class is responsible for retrieving requests, sending query requests, processing iterators,
 * and creating links using the LCAService. It also handles loading and saving configurations and
 * request buffers, and managing the lifecycle of the agent.
 */
class LinkCreationAgent {
   public:
    LinkCreationAgent(string config_path);
    LinkCreationAgent();
    ~LinkCreationAgent();

    /**
     * @brief Retrieve a request from DAS Node server or get a request from the requests buffer,
     * send a query request using DAS Node client, retrieve remote iterator and
     * send to LCAService to process the iterator and create links.
     */
    void run();
    /**
     * @brief Create the create link request.
     * @param request Request to be handled
     */
    static shared_ptr<LinkCreationAgentRequest> create_request(vector<string> request);


    void process_request(vector<string> request);

    void abort_request(const string& request_id);


   private:
    /**
     * @brief Sends a query to DAS Query Agent
     * @returns Returns a shared_ptr<PatternMatchingQueryProxy>, to iterate through the results
     */
    shared_ptr<PatternMatchingQueryProxy> query(vector<string>& query_tokens,
                                                string context,
                                                bool update_attention_broker);
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
     * @brief Stop the agent
     */
    void stop();

    // Attributes loaded from config file
    string config_path;                    // Path to the configuration file
    int requests_interval_seconds;         // Default interval to send requests
    int link_creation_agent_thread_count;  // Number of threads to process requests
    int query_timeout_seconds;             // Timeout for query requests
    string query_agent_client_id;          // ID of the query node client
    string query_agent_server_id;          // ID of the query node server
    string link_creation_agent_server_id;  // ID of the link creation server
    string das_agent_client_id;            // ID of the DAS client
    string das_agent_server_id;
    string requests_buffer_file;           // Path to the requests buffer file
    string metta_file_path = ".";          // Path to the metta file
    bool save_links_to_db = false;         // Save links on DB
    bool save_links_to_metta_file = true;  // Save links on file
    unsigned int query_agent_client_start_port;
    unsigned int query_agent_client_end_port;

    // Other attributes
    LinkCreationService* service;
    map<string, shared_ptr<LinkCreationAgentRequest>> request_buffer;
    LinkCreationAgentNode* link_creation_node_server;
    Queue<shared_ptr<LinkCreationAgentRequest>> requests_queue;  // Queue to hold requests
    DasAgentNode* das_client;
    thread* agent_thread;
    mutex agent_mutex;
    bool is_stoping = false;
    bool is_running = false;

    int loop_interval = 100;  // miliseconds
};
}  // namespace link_creation_agent
