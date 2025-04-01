#include "link_creation_agent.h"

#include <fstream>
#include <sstream>

#include "RemoteIterator.h"
#include "expression_hasher.h"
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
    service = new LinkCreationService(link_creation_agent_thread_count, shared_ptr<DASNode>(query_node_client));
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
        shared_ptr<LinkCreationAgentRequest> lca_request = NULL;
        if (!link_creation_node_server->is_query_empty()) {
            vector<string> request = link_creation_node_server->pop_request();
            lca_request = create_request(request);
            lca_request->current_interval = requests_interval_seconds;
            if (lca_request != NULL && (lca_request->infinite || lca_request->repeat > 0)) {
                request_buffer[lca_request->id] = lca_request;
            }
        } else {
            if (!request_buffer.empty()) {
                current_buffer_position = current_buffer_position % request_buffer.size();
                auto it = request_buffer.begin();
                advance(it, current_buffer_position);
                lca_request = it->second;
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
            if (request_buffer.find(lca_request->id) != request_buffer.end()) {
                request_buffer.erase(lca_request->id);
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
    for (auto it = request_buffer.begin(); it != request_buffer.end(); it++) {
        file.write((char*) &it->second, sizeof(it->second));
    }
    file.close();
}

void LinkCreationAgent::load_buffer() {
    ifstream file(requests_buffer_file, ios::binary);
    while (file) {
        LinkCreationAgentRequest request;
        file.read((char*) &request, sizeof(request));
        request_buffer[request.id] = make_shared<LinkCreationAgentRequest>(request);
    }
    file.close();
}

static bool is_link_template_arg(string arg) {
    if (arg == "LIST") return true;
    if (arg == "LINK_CREATE") return true;
    if (arg == "PROOF_OF_IMPLICATION_OR_EQUIVALENCE") return true;
    if (arg == "PROOF_OF_IMPLICATION") return true;
    if (arg == "PROOF_OF_EQUIVALENCE") return true;
    return false;
}

shared_ptr<LinkCreationAgentRequest> LinkCreationAgent::create_request(vector<string> request) {
    try {
        LinkCreationAgentRequest* lca_request = new LinkCreationAgentRequest();
        int cursor = 0;
        bool is_link_create = false;
        int has_id = 0;
        if (request[request.size() - 1] != "true" && request[request.size() - 1] != "false") {
            cout << "ID: " << request[request.size()-1] << endl;
            has_id = 1;
        }
        for (string arg : request) {
            cursor++;
            is_link_create = is_link_template_arg(arg) || is_link_create;
            if (!is_link_create) {
                lca_request->query.push_back(arg);
            }
            if (is_link_create && cursor < request.size() - 3 - has_id) {
                lca_request->link_template.push_back(arg);
            }
            if (cursor == request.size() - 3 - has_id) {
                lca_request->max_results = stoi(arg);
            }
            if (cursor == request.size() - 2 - has_id) {
                lca_request->repeat = stoi(arg);
            }
            if (cursor == request.size() - 1 - has_id) {
                lca_request->context = arg;
            }
                if (cursor == request.size() - has_id) {
                lca_request->update_attention_broker = (arg == "true");
            }else{
                if (cursor == request.size()) {
                    lca_request->id = arg;
                }
            }
        }
        lca_request->infinite = (lca_request->repeat == -1);
        if (lca_request->id.empty()){
            string temp_id = to_string(time(0)) + Utils::random_string(20);
            shared_ptr<char> temp_id_c = shared_ptr<char>(new char[temp_id.length() + 1], [](char* p) { delete[] p; });
            strcpy(temp_id_c.get(), temp_id.c_str());
            lca_request->id = compute_hash(temp_id_c.get());
        }
        // couts
        cout << "Query: " << Utils::join(lca_request->query, ' ') << endl;
        cout << "Link Template: " << Utils::join(lca_request->link_template, ' ') << endl;
        cout << "Max Results: " << lca_request->max_results << endl;
        cout << "Repeat: " << lca_request->repeat << endl;
        cout << "Context: " << lca_request->context << endl;
        cout << "Update Attention Broker: " << lca_request->update_attention_broker << endl;
        cout << "Infinite: " << lca_request->infinite << endl;
        cout << "ID: " << lca_request->id << endl;

        return shared_ptr<LinkCreationAgentRequest>(lca_request);
    } catch (exception& e) {
        cout << "Error parsing request: " << e.what() << endl;
        return NULL;
    }
}