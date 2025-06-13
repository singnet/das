#include "LinkCreationAgent.h"

#include <fstream>
#include <sstream>

#include "LinkCreateTemplate.h"
#include "Logger.h"
#include "expression_hasher.h"

using namespace std;
using namespace link_creation_agent;

LinkCreationAgent::LinkCreationAgent(string config_path) {
    this->config_path = config_path;
    load_config();
    link_creation_node_server = new LinkCreationAgentNode(link_creation_agent_server_id);
    ServiceBusSingleton::init(query_agent_client_id,
                              query_agent_server_id,
                              query_agent_client_start_port,
                              query_agent_client_end_port);
    service = new LinkCreationService(link_creation_agent_thread_count);
    service->set_timeout(query_timeout_seconds);
    service->set_metta_file_path(metta_file_path);
    if (save_links_to_metta_file) {
        LOG_DEBUG("Saving links to metta file: " << metta_file_path);
    }
    if (save_links_to_db) {
        LOG_DEBUG("Saving links to DB");
    }
    service->set_save_links_to_metta_file(save_links_to_metta_file);
    service->set_save_links_to_db(save_links_to_db);
    das_client = new DasAgentNode(das_agent_client_id, das_agent_server_id);
    this->agent_thread = new thread(&LinkCreationAgent::run, this);
}

LinkCreationAgent::~LinkCreationAgent() {
    stop();
    delete link_creation_node_server;
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
    das_client->graceful_shutdown();
    if (agent_thread != NULL && agent_thread->joinable()) {
        agent_thread->join();
        agent_thread = NULL;
    }
}

void LinkCreationAgent::run() {
    int current_buffer_position = 0;
    if (is_running) return;
    is_running = true;
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

        if (lca_request == nullptr ||
            lca_request->last_execution + lca_request->current_interval > time(0)) {
            Utils::sleep(loop_interval);
            continue;
        }

        if (lca_request->infinite || lca_request->repeat > 0) {
            // query_agent_mutex->lock();
            LOG_DEBUG("Request IDX: " << current_buffer_position);
            LOG_DEBUG("Processing request ID: " << lca_request->id);
            LOG_DEBUG("Current size of request buffer: " << request_buffer.size());
            shared_ptr<PatternMatchingQueryProxy> proxy =
                query(lca_request->query, lca_request->context, lca_request->update_attention_broker);

            service->process_request(proxy,
                                     das_client,
                                     lca_request->link_template,
                                     lca_request->context,
                                     lca_request->id,
                                     lca_request->max_results);
            lca_request->last_execution = time(0);
            if (lca_request->infinite) continue;
            if (lca_request->repeat >= 1) lca_request->repeat--;

        } else {
            if (request_buffer.find(lca_request->id) != request_buffer.end()) {
                LOG_DEBUG("Removing request ID: " << lca_request->id);
                request_buffer.erase(lca_request->id);
            }
        }
    }
}

shared_ptr<PatternMatchingQueryProxy> LinkCreationAgent::query(vector<string>& query_tokens,
                                                               string context,
                                                               bool update_attention_broker) {
    shared_ptr<PatternMatchingQueryProxy> proxy =
        make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    proxy->set_attention_update_flag(update_attention_broker);
    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    return proxy;
}

void LinkCreationAgent::load_config() {
    map<string, string> config_map = Utils::parse_config(config_path);
    this->requests_interval_seconds = stoi(config_map["requests_interval_seconds"]);
    this->link_creation_agent_thread_count = stoi(config_map["link_creation_agent_thread_count"]);
    this->query_agent_client_id = config_map["query_agent_client_id"];
    this->query_agent_server_id = config_map["query_agent_server_id"];
    this->link_creation_agent_server_id = config_map["link_creation_agent_server_id"];
    this->das_agent_client_id = config_map["das_agent_client_id"];
    this->requests_buffer_file = config_map["requests_buffer_file"];
    this->das_agent_server_id = config_map["das_agent_server_id"];
    this->query_agent_client_start_port = stoul(config_map["query_agent_client_start_port"]);
    this->query_agent_client_end_port = stoul(config_map["query_agent_client_end_port"]);
    this->query_timeout_seconds = stoi(config_map["query_timeout_seconds"]);
    this->metta_file_path = config_map["metta_file_path"];
    this->save_links_to_db = config_map.find("save_links_to_db") != config_map.end()
                                 ? config_map["save_links_to_db"] == "true"
                                 : this->save_links_to_db;
    this->save_links_to_metta_file = config_map.find("save_links_to_metta_file") != config_map.end()
                                         ? config_map["save_links_to_metta_file"] == "true"
                                         : this->save_links_to_metta_file;
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
        long unsigned int cursor = 0;
        bool is_link_create = false;
        int has_id = 0;
        if (request[request.size() - 1] != "true" && request[request.size() - 1] != "false") {
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
            } else {
                if (cursor == request.size()) {
                    lca_request->id = arg;
                }
            }
        }
        lca_request->infinite = (lca_request->repeat == -1);
        if (lca_request->id.empty()) {
            lca_request->id =
                compute_hash((char*) (to_string(time(0)) + Utils::random_string(20)).c_str());
        } else {
            lca_request->id = lca_request->id + "-" +
                              compute_hash((char*) Utils::join(lca_request->link_template, ' ').c_str());
        }
        LOG_DEBUG("Creating request ID: " << lca_request->id);
        LOG_DEBUG("Query: " << Utils::join(lca_request->query, ' '));
        LOG_DEBUG("Link Template: " << Utils::join(lca_request->link_template, ' '));
        LOG_DEBUG("Max Results: " << to_string(lca_request->max_results));
        LOG_DEBUG("Repeat: " << to_string(lca_request->repeat));
        LOG_DEBUG("Context: " << lca_request->context);
        LOG_DEBUG("Update Attention Broker: " << to_string(lca_request->update_attention_broker));
        LOG_DEBUG("Infinite: " << to_string(lca_request->infinite));

        return shared_ptr<LinkCreationAgentRequest>(lca_request);
    } catch (exception& e) {
        LOG_ERROR("Error parsing request: " << string(e.what()));

        return NULL;
    }
}
