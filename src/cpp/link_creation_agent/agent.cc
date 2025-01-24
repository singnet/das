#include "agent.h"

#include <fstream>
#include <sstream>

using namespace std;
using namespace link_creation_agent;
using namespace query_node;

LinkCreationAgent::LinkCreationAgent(string config_path) {
    this->config_path = config_path;
    load_config();
    link_creation_node_server = new LinkCreationNode(link_creation_server_id);
    this->agent_thread = new thread(&LinkCreationAgent::run, this);
}

LinkCreationAgent::~LinkCreationAgent() {
    stop();
    delete link_creation_node_server;
}

void LinkCreationAgent::stop() {
    link_creation_node_server->graceful_shutdown();
    if (agent_thread != NULL && agent_thread->joinable()) {
        agent_thread->join();
        agent_thread = NULL;
    }
}

void LinkCreationAgent::run() {
    while (true) {
        this_thread::sleep_for(chrono::seconds(loop_interval));
        if (!link_creation_node_server->is_query_empty()) {
            auto request = link_creation_node_server->pop_request();
            cout << "Request: " << request << endl;
        }
        cout << "Running" << endl;
    }
}

void LinkCreationAgent::clean_requests() {}

shared_ptr<RemoteIterator> LinkCreationAgent::query() { return NULL; }

void LinkCreationAgent::load_config() {
    ifstream file(config_path);
    string line;
    while (getline(file, line)) {
        istringstream is_line(line);
        string key;
        if (getline(is_line, key, '=')) {
            string value;
            if (getline(is_line, value)) {
                value.erase(remove(value.begin(), value.end(), ' '), value.end());
                key.erase(remove(key.begin(), key.end(), ' '), key.end());
                if (key == "default_interval")
                    this->default_interval = stoi(value);
                else if (key == "thread_count")
                    this->thread_count = stoi(value);
                else if (key == "query_node_client_id")
                    this->query_node_client_id = value;
                else if (key == "query_node_server_id")
                    this->query_node_server_id = value;
                else if (key == "link_creation_server_id")
                    this->link_creation_server_id = value;
                else if (key == "das_client_id")
                    this->das_client_id = value;
            }
        }
    }
    file.close();
}

void LinkCreationAgent::save_buffer() {}

void LinkCreationAgent::load_buffer() {}

void LinkCreationAgent::handleRequest(string request) {}