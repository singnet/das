#include "InferenceAgent.h"

#include <limits.h>

#include <fstream>
#include <sstream>
#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

using namespace std;
using namespace inference_agent;
using namespace link_creation_agent;

const std::string InferenceAgent::PROOF_OF_IMPLICATION = "PROOF_OF_IMPLICATION";
const std::string InferenceAgent::PROOF_OF_EQUIVALENCE = "PROOF_OF_EQUIVALENCE";

InferenceAgent::InferenceAgent() { this->agent_thread = new thread(&InferenceAgent::run, this); }

InferenceAgent::~InferenceAgent() {
    stop();
    if (agent_thread != nullptr && agent_thread->joinable()) agent_thread->join();
    if (agent_thread != nullptr) delete agent_thread;
}

shared_ptr<InferenceRequest> InferenceAgent::build_inference_request(const vector<string>& request) {
    if (!inference_request_validator.validate(request)) {
        Utils::error("Invalid inference request");
    }
    shared_ptr<InferenceRequest> inference_request;
    string inference_command = request.front();
    string first_handle = request[1];
    string second_handle = request[2];
    int max_proof_length = Utils::string_to_int(request[3]);
    if (max_proof_length > max_proof_length_limit) {
        Utils::error("Max proof length exceeded");
    }
    string context = request[4];
    if (inference_command == PROOF_OF_IMPLICATION) {
        LOG_DEBUG("Received proof of implication");
        inference_request =
            make_shared<ProofOfImplication>(first_handle, second_handle, max_proof_length, context);
    } else if (inference_command == PROOF_OF_EQUIVALENCE) {
        LOG_DEBUG("Received proof of equivalence");
        inference_request =
            make_shared<ProofOfEquivalence>(first_handle, second_handle, max_proof_length, context);
    } else {
        Utils::error("Invalid inference command");
    }
    return inference_request;
}

void InferenceAgent::run() {
    LOG_DEBUG("Inference agent is running");
    shared_ptr<QueryAnswer> query_answer;
    while (true) {
        if (is_stoping) break;
        if (!inference_request_queue.empty()) {
            try {
                auto inference_request = inference_request_queue.dequeue();
                send_link_creation_request(inference_request);
                this->inference_requests.push_back(inference_request);
                inference_timeout_map[inference_request->get_id()] =
                    (Utils::get_current_time_millis() / 1000) + inference_request->get_timeout();
            } catch (const exception& e) {
                LOG_ERROR("Exception: " << e.what());
            }
        }
        // Check if there are any evolution proxies to process
        for (auto it = evolution_proxy_map.begin(); it != evolution_proxy_map.end();) {
            if (it->second->finished() || inference_proxy_map[it->first]->is_aborting() ||
                Utils::get_current_time_millis() / 1000 > inference_timeout_map[it->first]) {
                LOG_DEBUG("Processing inference abort request for request ID: " << it->first);

                auto inference_request = get_inference_request(it->first);
                if (inference_request != nullptr) {
                    if (inference_request->get_repeat() > 0) {
                        LOG_DEBUG("Sending link creation request for request ID: "
                                  << inference_request->get_id());
                        LOG_DEBUG("Request repeat count: " << inference_request->get_repeat());
                        send_link_creation_request(inference_request);
                        inference_request->set_sent_evolution_request(false);
                    } else {
                        this->inference_requests.erase(remove(this->inference_requests.begin(),
                                                              this->inference_requests.end(),
                                                              inference_request),
                                                       this->inference_requests.end());
                        LOG_DEBUG("Inference request removed for request ID: " << it->first);
                        process_inference_abort_request(it->first);
                    }
                }

                it = evolution_proxy_map.erase(it);

            } else {
                if ((query_answer = it->second->pop()) != NULL) {
                    inference_proxy_map[it->first]->push(query_answer);
                }
                it++;
            }
        }

        for (auto& inference_request : this->inference_requests) {
            if (is_lca_requests_finished(inference_request) &&
                !inference_request->get_sent_evolution_request()) {
                LOG_DEBUG(
                    "LCA requests finished for inference request ID: " << inference_request->get_id());
                send_distributed_inference_control_request(inference_request);
                inference_request->set_sent_evolution_request(true);
                inference_request->set_repeat(inference_request->get_repeat() - 1);
            }
        }
        Utils::sleep();
    }
}

void InferenceAgent::stop() {
    agent_mutex.lock();
    if (!is_stoping) {
        is_stoping = true;
    }
    agent_mutex.unlock();
}

bool InferenceAgent::is_lca_requests_finished(shared_ptr<InferenceRequest> inference_request) {
    auto it = link_creation_proxy_map.find(inference_request->get_id());
    if (it == link_creation_proxy_map.end()) {
        Utils::error("No link creation requests found for inference request ID: " +
                     inference_request->get_id());
    }
    for (const auto& proxy : it->second) {
        if (!proxy->finished()) {
            return false;
        }
    }
    return true;
}

shared_ptr<InferenceRequest> InferenceAgent::get_inference_request(const string& request_id) {
    auto it = find_if(
        inference_requests.begin(),
        inference_requests.end(),
        [&request_id](const shared_ptr<InferenceRequest>& req) { return req->get_id() == request_id; });
    if (it != inference_requests.end()) {
        return *it;
    }
    return nullptr;
}

void InferenceAgent::send_link_creation_request(shared_ptr<InferenceRequest> inference_request) {
    LOG_DEBUG("Sending link creation request for inference request ID: " << inference_request->get_id());
    this->link_creation_proxy_map[inference_request->get_id()] =
        vector<shared_ptr<LinkCreationRequestProxy>>();
    auto service_bus = ServiceBusSingleton::get_instance();
    for (auto& request_iterator : inference_request->get_requests()) {
        vector<string> request;
        for (auto& token : request_iterator) {
            request.push_back(token);
        }
        request.push_back(
            to_string(inference_request->get_lca_max_results()));  // TODO check max results value
        request.push_back(to_string(inference_request->get_lca_max_repeats()));  // repeat
        request.push_back(inference_request->get_context());                     // context
        request.push_back(inference_request->get_lca_update_attention_broker()
                              ? "true"
                              : "false");  // update_attention_broker
        auto link_creation_proxy = make_shared<LinkCreationRequestProxy>(request);
        this->link_creation_proxy_map[inference_request->get_id()].push_back(link_creation_proxy);
        service_bus->issue_bus_command(link_creation_proxy);
    }
}

void InferenceAgent::send_distributed_inference_control_request(
    shared_ptr<InferenceRequest> inference_request) {
    string request_id = inference_request->get_id();
    LOG_DEBUG("Sending distributed inference control request ID: " << request_id);
    auto evolution_request = inference_request->get_distributed_inference_control_request();
    LOG_DEBUG("Distributed inference control request: " << Utils::join(evolution_request, ' '));
    QueryEvolutionProxy* evolution_proxy_ptr = new QueryEvolutionProxy(
        evolution_request, {}, {}, inference_request->get_context(), "multiply_strength");
    shared_ptr<QueryEvolutionProxy> evolution_proxy(evolution_proxy_ptr);
    ServiceBusSingleton::get_instance()->issue_bus_command(evolution_proxy);
    evolution_proxy_map[request_id] = evolution_proxy;
    LOG_DEBUG("Distributed inference control request sent");
}

void InferenceAgent::process_inference_request(const vector<string>& request, const string& request_id) {
    if (request.empty()) {
        Utils::error("Empty inference request");
    }
    LOG_DEBUG("Processing inference request: " << Utils::join(request, ' '));
    auto inference_request = build_inference_request(request);
    inference_request->set_id(request_id);
    inference_request_queue.enqueue(inference_request);
}

void InferenceAgent::process_inference_request(shared_ptr<InferenceProxy> proxy) {
    if (proxy == nullptr) {
        Utils::error("Invalid inference proxy");
    }
    LOG_DEBUG("Processing inference request: " << Utils::join(proxy->get_args(), ' '));
    string request_id = proxy->peer_id() + to_string(proxy->get_serial());
    inference_proxy_map[request_id] = proxy;
    if (proxy->get_args().empty()) {
        Utils::error("Empty inference request");
    }
    LOG_DEBUG("Inference Request:");
    LOG_DEBUG("  Request ID: " << request_id);
    LOG_DEBUG("  Tokens: " << Utils::join(proxy->get_args(), ' '));
    LOG_DEBUG(
        "  Timeout: " << proxy->parameters.get<unsigned int>(InferenceProxy::INFERENCE_REQUEST_TIMEOUT));
    LOG_DEBUG("  Max Results: " << proxy->parameters.get<unsigned int>(
                  InferenceProxy::MAX_QUERY_ANSWERS_TO_PROCESS));
    LOG_DEBUG(
        "  Full Evaluation: " << proxy->parameters.get<bool>(InferenceProxy::RUN_FULL_EVALUATION_QUERY));
    auto inference_request = build_inference_request(proxy->get_args());
    inference_request->set_id(request_id);
    inference_request->set_timeout(
        proxy->parameters.get<unsigned int>(InferenceProxy::INFERENCE_REQUEST_TIMEOUT));
    inference_request->set_lca_max_results(
        proxy->parameters.get<unsigned int>(InferenceProxy::MAX_QUERY_ANSWERS_TO_PROCESS));
    inference_request->set_full_evaluation(
        proxy->parameters.get<bool>(InferenceProxy::RUN_FULL_EVALUATION_QUERY));
    inference_request->set_lca_update_attention_broker(
        proxy->parameters.get<bool>(InferenceProxy::UPDATE_ATTENTION_BROKER_FLAG));
    inference_request_queue.enqueue(inference_request);
    LOG_DEBUG("Inference request processed for request ID: " << request_id);
}

void InferenceAgent::process_inference_abort_request(const string& request_id) {
    LOG_DEBUG("Evolution proxy finished for request ID: " << request_id);
    for (auto& link_creation_proxy : link_creation_proxy_map[request_id]) {
        LOG_DEBUG("Aborting link creation proxy for request ID: " << request_id);
        link_creation_proxy->abort();
    }
    link_creation_proxy_map.erase(request_id);
    LOG_DEBUG("Link creation request aborted for request ID: " << request_id);
    inference_proxy_map[request_id]->query_processing_finished();
    LOG_DEBUG("Inference proxy query processing finished for request ID: " << request_id);
    inference_proxy_map.erase(request_id);
    if (!evolution_proxy_map[request_id]->finished()) {
        LOG_DEBUG("Evolution proxy not finished for request ID: " << request_id);
        evolution_proxy_map[request_id]->abort();
        LOG_DEBUG("Evolution proxy aborted for request ID: " << request_id);
    }
    LOG_DEBUG("Inference request processed and cleaned up for request ID: " << request_id);
}
