#include "InferenceAgent.h"

#include <limits.h>

#include <fstream>
#include <sstream>

#include "Logger.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

using namespace std;
using namespace inference_agent;
using namespace link_creation_agent;

const std::string InferenceAgent::PROOF_OF_IMPLICATION_OR_EQUIVALENCE =
    "PROOF_OF_IMPLICATION_OR_EQUIVALENCE";
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
    if (inference_command == PROOF_OF_IMPLICATION_OR_EQUIVALENCE) {
        LOG_DEBUG("Received proof of implication or equivalence");
        inference_request = make_shared<ProofOfImplicationOrEquivalence>(
            first_handle, second_handle, max_proof_length, context);
    } else if (inference_command == PROOF_OF_IMPLICATION) {
        LOG_DEBUG("Received proof of implication");
        inference_request =
            make_shared<ProofOfImplication>(first_handle, second_handle, max_proof_length, context);
    } else if (inference_command == PROOF_OF_EQUIVALENCE) {
        LOG_DEBUG("Received proof of equivalence");
        inference_request =
            make_shared<ProofOfEquivalence>(first_handle, second_handle, max_proof_length, context);
    }
    return inference_request;
}

void InferenceAgent::run() {
    LOG_DEBUG("Inference agent is running");
    while (true) {
        if (is_stoping) break;
        if (!inference_request_queue.empty()) {
            try {
                auto inference_request = inference_request_queue.dequeue();
                send_link_creation_request(inference_request);
                send_distributed_inference_control_request(inference_request->get_id());
            } catch (const std::exception& e) {
                LOG_ERROR("Exception: " << e.what());
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
        request.push_back("1000000");                         // TODO check max results value
        request.push_back("-1");                              // repeat
        request.push_back(inference_request->get_context());  // context
        request.push_back("false");                           // update_attention_broker
        auto link_creation_proxy = make_shared<LinkCreationRequestProxy>(request);
        this->link_creation_proxy_map[inference_request->get_id()].push_back(link_creation_proxy);
        service_bus->issue_bus_command(link_creation_proxy);
    }
}

void InferenceAgent::send_stop_link_creation_request(const string& request_id) {
    for (auto& proxy : this->link_creation_proxy_map[request_id]) {
        proxy->set_parameter(LinkCreationRequestProxy::Parameters::ABORT_FLAG, true);
        ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    }
}

void InferenceAgent::send_distributed_inference_control_request(const string& client_node_id) {
    LOG_DEBUG("Sending distributed inference control request ID: " << client_node_id);
    // TODO add evolution command
    // auto evolution_proxy = make_shared<QueryEvolutionProxy>();
    // ServiceBusSingleton::get_instance()->issue_bus_command(evolution_proxy);
    // ...
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

void InferenceAgent::process_inference_abort_request(const string& request_id) {
    LOG_DEBUG("Processing inference abort request: " << request_id);
    if (link_creation_proxy_map.find(request_id) != link_creation_proxy_map.end()) {
        send_stop_link_creation_request(request_id);
        link_creation_proxy_map.erase(request_id);
    } else {
        LOG_ERROR("No inference request found for ID: " << request_id);
    }
}
