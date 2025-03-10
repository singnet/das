#include "inference_agent.h"

#include <fstream>
#include <sstream>

#include "Utils.h"

using namespace std;
using namespace inference_agent;

const std::string InferenceAgent::PROOF_OF_IMPLICATION_OR_EQUIVALENCE =
    "PROOF_OF_IMPLICATION_OR_EQUIVALENCE";
const std::string InferenceAgent::PROOF_OF_IMPLICATION = "PROOF_OF_IMPLICATION";
const std::string InferenceAgent::PROOF_OF_EQUIVALENCE = "PROOF_OF_EQUIVALENCE";

InferenceAgent::InferenceAgent(const string& config_path) {
    parse_config(config_path);
    inference_node_server = new InferenceAgentNode(inference_node_id);
    link_creation_node_client =
        new LinkCreationAgentNode(link_creation_agent_client_id, link_creation_agent_server_id);
    das_client = new DasAgentNode(das_client_id, das_server_id);
    distributed_inference_control_client = new DistributedInferenceControlAgentNode(
        distributed_inference_control_node_id, distributed_inference_control_node_server_id);
    vector<string> host_port = Utils::split(inference_node_id, ':');
    if (host_port.size() != 2) {
        Utils::error("Invalid inference node id");
    }
    inference_node_server_host = host_port[0];
    inference_node_server_port = host_port[1];
    thread_pool = new ThreadPool(iterator_pool_size);
    this->agent_thread = new thread(&InferenceAgent::run, this);
}

InferenceAgent::~InferenceAgent() {
    stop();
    inference_node_server->graceful_shutdown();
    delete inference_node_server;
    link_creation_node_client->graceful_shutdown();
    delete link_creation_node_client;
    das_client->graceful_shutdown();
    delete das_client;
    distributed_inference_control_client->graceful_shutdown();
    delete distributed_inference_control_client;
}

void InferenceAgent::run() {
    cout << "Inference agent is running" << endl;
    while (true) {
        if (is_stoping) break;
        if (!inference_node_server->is_answers_empty()) {
            auto answer = inference_node_server->pop_answer();
            try {
                if (inference_request_validator.validate(answer)) {
                    shared_ptr<InferenceRequest> inference_request;
                    if (answer.front() == PROOF_OF_IMPLICATION_OR_EQUIVALENCE) {
                        cout << "Received proof of implication or equivalence" << endl;
                        inference_request = make_shared<ProofOfImplicationOrEquivalence>(
                            answer[1], answer[2], stoi(answer[3]));
                    } else if (answer.front() == PROOF_OF_IMPLICATION) {
                        cout << "Received proof of implication" << endl;
                        inference_request =
                            make_shared<ProofOfImplication>(answer[1], answer[2], stoi(answer[3]));
                    } else if (answer.front() == PROOF_OF_EQUIVALENCE) {
                        cout << "Received proof of equivalence" << endl;
                        inference_request =
                            make_shared<ProofOfEquivalence>(answer[1], answer[2], stoi(answer[3]));
                    }
                    string iterator_id = get_next_iterator_id();
                    iterator_link_creation_request_map[iterator_id] = inference_request;  // iterator id
                    send_link_creation_request(inference_request, false);
                    send_distributed_inference_control_request(iterator_id);
                } else {
                    Utils::error("Invalid inference request");
                }
            } catch (const std::exception& e) {
                cout << "Error: " << e.what() << endl;
            }
        } else {
            for (int i = 0; i < inference_iterators.size(); i++) {
                if (!inference_iterators[i]->pop(false).empty()) {
                    send_stop_link_creation_request(
                        iterator_link_creation_request_map[inference_iterators[i]->get_local_id()]);
                    inference_iterators.erase(inference_iterators.begin() + i);
                }
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

void InferenceAgent::send_link_creation_request(shared_ptr<InferenceRequest> inference_request,
                                                bool is_stop_request) {
    for (auto& request_iterator : inference_request->get_requests()) {
        vector<string> request;
        for (auto& token : request_iterator) {
            request.push_back(token);
        }
        request.push_back("1000");                        // TODO check max results value
        request.push_back(is_stop_request ? "0" : "-1");  // repeat
        request.push_back(inference_request->get_id());   // context
        request.push_back("false");                       // update_attention_broker
        link_creation_node_client->send_message(request);
    }
}

void InferenceAgent::send_stop_link_creation_request(shared_ptr<InferenceRequest> inference_request) {
    send_link_creation_request(inference_request, true);
}

void InferenceAgent::send_distributed_inference_control_request(const string& client_node_id) {
    shared_ptr<InferenceIterator<InferenceAgentNode>> inference_iterator =
        make_shared<InferenceIterator<InferenceAgentNode>>(client_node_id);
    inference_iterators.push_back(inference_iterator);
    distributed_inference_control_client->send_inference_control_request(
        iterator_link_creation_request_map[client_node_id]->get_distributed_inference_control_request());
}

vector<string> InferenceAgent::get_link_creation_request() { return vector<string>(); }

const string InferenceAgent::get_next_iterator_id() {
    string last_part;
    if (current_iterator_id == 0) {
        current_iterator_id = stoi(inference_node_server_port);
    }
    // increment the last part
    current_iterator_id = current_iterator_id % 65535 + 1;
    last_part = to_string(current_iterator_id);
    return inference_node_server_host + ":" + last_part;
}

void InferenceAgent::parse_config(const string& config_path) {
    cout << "Parsing config file: " << config_path << endl;
    map<string, string> config = Utils::parse_config(config_path);

    this->inference_node_id = config["inference_node_id"];
    this->das_client_id = config["das_client_id"];
    this->das_server_id = config["das_server_id"];
    this->distributed_inference_control_node_id = config["distributed_inference_control_node_id"];
    this->distributed_inference_control_node_server_id =
        config["distributed_inference_control_node_server_id"];
    this->link_creation_agent_server_id = config["link_creation_agent_server_id"];
    this->link_creation_agent_client_id = config["link_creation_agent_client_id"];
}
