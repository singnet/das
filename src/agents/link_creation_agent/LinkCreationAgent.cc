#include "LinkCreationAgent.h"

#include <fstream>

#include "LinkCreateTemplate.h"
#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"
#include "expression_hasher.h"

using namespace std;
using namespace link_creation_agent;

LinkCreationAgent::LinkCreationAgent(int request_interval,
                                     int thread_count,
                                     int default_timeout,
                                     string buffer_file_path,
                                     string metta_file_path,
                                     bool save_links_to_metta_file = true,
                                     bool save_links_to_db = false) {
    this->requests_interval_seconds = request_interval;
    this->link_creation_agent_thread_count = thread_count;
    this->query_timeout_seconds = default_timeout;
    this->requests_buffer_file = buffer_file_path;
    this->metta_file_path = metta_file_path;
    this->save_links_to_metta_file = save_links_to_metta_file;
    this->save_links_to_db = save_links_to_db;
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
    this->agent_thread = new thread(&LinkCreationAgent::run, this);
}

LinkCreationAgent::~LinkCreationAgent() {
    stop();
    delete service;
}

void LinkCreationAgent::stop() {
    agent_mutex.lock();
    if (!is_stoping) {
        is_stoping = true;
        save_buffer();
    }
    agent_mutex.unlock();
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
        shared_ptr<LinkCreationAgentRequest> lca_request = nullptr;
        if (!requests_queue.empty()) {
            lca_request = requests_queue.dequeue();
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

        LOG_DEBUG("LCArequest ID: " << (lca_request ? lca_request->id : "NULL"));

        if (lca_request->infinite || lca_request->repeat > 0) {
            LOG_DEBUG("Request IDX: " << current_buffer_position);
            LOG_DEBUG("Processing request ID: " << lca_request->id);
            LOG_DEBUG("Current size of request buffer: " << request_buffer.size());
            shared_ptr<PatternMatchingQueryProxy> proxy =
                query(lca_request->query, lca_request->context, lca_request->update_attention_broker);
            pattern_query_proxy_map[lca_request->id] = proxy;

            service->process_request(proxy,
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
        for (auto& [request_id, proxy] : link_creation_proxy_map) {
            if (proxy->is_aborting()) {
                abort_request(request_id);
                link_creation_proxy_map.erase(request_id);
            }
        }
    }
}

shared_ptr<PatternMatchingQueryProxy> LinkCreationAgent::query(vector<string>& query_tokens,
                                                               string context,
                                                               bool update_attention_broker) {
    shared_ptr<PatternMatchingQueryProxy> proxy =
        make_shared<PatternMatchingQueryProxy>(query_tokens, context);
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = update_attention_broker;
    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    return proxy;
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
        lca_request->id = compute_hash((char*) lca_request->id.c_str());
        LOG_DEBUG("Creating request ID: " << lca_request->id);
        LOG_DEBUG("Query: " << Utils::join(lca_request->query, ' '));
        LOG_DEBUG("LCALink Template: " << Utils::join(lca_request->link_template, ' '));
        LOG_DEBUG("Max Results: " << to_string(lca_request->max_results));
        LOG_DEBUG("Repeat: " << to_string(lca_request->repeat));
        LOG_DEBUG("Context: " << lca_request->context);
        LOG_DEBUG("Update Attention Broker: " << to_string(lca_request->update_attention_broker));
        LOG_DEBUG("Infinite: " << to_string(lca_request->infinite));

        return shared_ptr<LinkCreationAgentRequest>(lca_request);
    } catch (exception& e) {
        LOG_ERROR("Error parsing request: " << string(e.what()));
        throw invalid_argument("Invalid request format: " + string(e.what()));
    }
    return nullptr;
}

void LinkCreationAgent::process_request(vector<string> request) {
    this->requests_queue.enqueue(create_request(request));
}

void LinkCreationAgent::process_request(shared_ptr<LinkCreationRequestProxy> proxy) {
    if (proxy == nullptr) {
        Utils::error("Invalid link creation request proxy");
    }
    LOG_DEBUG("Processing link creation request: " << Utils::join(proxy->get_args(), ' '));
    string request_id = proxy->peer_id() + to_string(proxy->get_serial());
    auto request = proxy->get_args();
    request.push_back(request_id);
    process_request(request);
    link_creation_proxy_map[request_id] = proxy;
    LOG_DEBUG("LCALink creation request processed for request ID: " << request_id);
}

void LinkCreationAgent::abort_request(const string& request_id) {
    lock_guard<mutex> lock(agent_mutex);
    string request_id_hash = compute_hash((char*) request_id.c_str());
    if (request_buffer.find(request_id_hash) != request_buffer.end()) {
        request_buffer.erase(request_id_hash);
        LOG_DEBUG("Aborted request ID: " << request_id_hash);
    } else {
        LOG_DEBUG("Request ID: " << request_id_hash << " not found in buffer");
    }
    pattern_query_proxy_map[request_id_hash]->abort();
    pattern_query_proxy_map.erase(request_id_hash);
}