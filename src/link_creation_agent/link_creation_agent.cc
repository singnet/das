#include "link_creation_agent.h"

#include <fstream>
#include <sstream>

#include "RemoteIterator.h"
#include "link_create_template.h"

using namespace std;
using namespace link_creation_agent;
using namespace query_node;
using namespace query_element;

LinkCreationAgent::LinkCreationAgent(string config_path) {
    this->config_path = config_path;
    load_config();
    link_creation_node_server = new LinkCreationAgentNode(link_creation_agent_server_id);
    query_node_client = new DASNode(query_agent_client_id, query_agent_server_id);
    service = new LinkCreationService(link_creation_agent_thread_count);
    das_client = new DasAgentNode(das_agent_client_id, das_agent_server_id);

    this->agent_thread = new thread(&LinkCreationAgent::run, this);
}

LinkCreationAgent::~LinkCreationAgent() {
    stop();
    delete link_creation_node_server;
    delete query_node_client;
    delete service;
    delete das_client;
}

void LinkCreationAgent::stop() {
    agent_mutex.lock();
    if (!is_stoping) {
        is_stoping = true;
        save_buffer();
    }
    agent_mutex.unlock();
    link_creation_node_server->graceful_shutdown();
    query_node_client->graceful_shutdown();
    das_client->graceful_shutdown();
    if (agent_thread != NULL && agent_thread->joinable()) {
        agent_thread->join();
        agent_thread = NULL;
    }
}

void LinkCreationAgent::run() {
    int current_buffer_position = 0;
    while (true) {
        if (is_stoping) break;
        LinkCreationAgentRequest* lca_request = NULL;
        bool is_from_buffer = false;
        if (!link_creation_node_server->is_query_empty()) {
            vector<string> request = link_creation_node_server->pop_request();
            lca_request = create_request(request);
            lca_request->current_interval = requests_interval_seconds;
            if (lca_request != NULL && (lca_request->infinite || lca_request->repeat > 0)) {
                request_buffer.push_back(*lca_request);
            }
        } else {
            if (!request_buffer.empty()) {
                is_from_buffer = true;
                current_buffer_position = current_buffer_position % request_buffer.size();
                lca_request = &request_buffer[current_buffer_position];
                current_buffer_position++;
            }
        }

        if (lca_request == NULL ||
            lca_request->last_execution + lca_request->current_interval > time(0)) {
            this_thread::sleep_for(chrono::milliseconds(loop_interval));
            continue;
        }

        shared_ptr<RemoteIterator<HandlesAnswer>> iterator =
            query(lca_request->query, lca_request->context, lca_request->update_attention_broker);

        service->process_request(
            iterator, das_client, lca_request->link_template, lca_request->max_results);

        if (lca_request->infinite || lca_request->repeat > 0) {
            lca_request->last_execution = time(0);
            lca_request->current_interval =
                (lca_request->current_interval * 2) %
                86400;  // TODO Add exponential backoff, resets after 24 hours

            if (lca_request->infinite) continue;
            if (lca_request->repeat >= 1) lca_request->repeat--;

        } else {
            if (is_from_buffer) {
                request_buffer.erase(request_buffer.begin() + current_buffer_position - 1);
            } else {
                delete lca_request;
            }
        }
    }
}

shared_ptr<RemoteIterator<HandlesAnswer>> LinkCreationAgent::query(vector<string>& query_tokens,
                                                                   string context,
                                                                   bool update_attention_broker) {
    return shared_ptr<RemoteIterator<HandlesAnswer>>(
        query_node_client->pattern_matcher_query(query_tokens, context, update_attention_broker));
}

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
                if (key == "requests_interval_seconds")
                    this->requests_interval_seconds = stoi(value);
                else if (key == "link_creation_agent_thread_count")
                    this->link_creation_agent_thread_count = stoi(value);
                else if (key == "query_agent_client_id")
                    this->query_agent_client_id = value;
                else if (key == "query_agent_server_id")
                    this->query_agent_server_id = value;
                else if (key == "link_creation_agent_server_id")
                    this->link_creation_agent_server_id = value;
                else if (key == "das_agent_client_id")
                    this->das_agent_client_id = value;
                else if (key == "requests_buffer_file")
                    this->requests_buffer_file = value;
                else if (key == "das_agent_server_id")
                    this->das_agent_server_id = value;
                else if (key == "context")
                    this->context = value;
            }
        }
    }
    file.close();
}

void LinkCreationAgent::save_buffer() {
    ofstream file(requests_buffer_file, ios::binary);
    for (LinkCreationAgentRequest request : request_buffer) {
        file.write((char*) &request, sizeof(request));
    }
    file.close();
}

void LinkCreationAgent::load_buffer() {
    ifstream file(requests_buffer_file, ios::binary);
    while (file) {
        LinkCreationAgentRequest request;
        file.read((char*) &request, sizeof(request));
        request_buffer.push_back(request);
    }
    file.close();
}

LinkCreationAgentRequest* LinkCreationAgent::create_request(vector<string> request) {
    try {
        LinkCreationAgentRequest* lca_request = new LinkCreationAgentRequest();
        int cursor = 0;
        bool is_link_create = false;
        for (string arg : request) {
            cursor++;
            is_link_create = (arg == "LINK_CREATE") || is_link_create;
            if (!is_link_create) {
                lca_request->query.push_back(arg);
            }
            if (is_link_create && cursor < request.size() - 3) {
                lca_request->link_template.push_back(arg);
            }
            if (cursor == request.size() - 3) {
                lca_request->max_results = stoi(arg);
            }
            if (cursor == request.size() - 2) {
                lca_request->repeat = stoi(arg);
            }
            if (cursor == request.size() - 1) {
                lca_request->context = arg;
            }
            if (cursor == request.size()) {
                lca_request->update_attention_broker = (arg == "true");
            }
        }
        lca_request->infinite = (lca_request->repeat == -1);
        return lca_request;
    } catch (exception& e) {
        cout << "Error parsing request: " << e.what() << endl;
        return NULL;
    }
}