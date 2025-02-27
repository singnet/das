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
    cout << "Initializing inference agent" << endl;
    parse_config(config_path);
    cout << "Starting inference node server" << endl;
    inference_node_server = new InferenceAgentNode(inference_node_id);
    cout << "Starting link creation node client" << endl;
    link_creation_node_client = new LinkCreationAgentNode(link_creation_agent_client_id);
    cout << "Starting das client" << endl;
    das_client = new DasAgentNode(das_client_id, das_server_id);
    cout << "Starting distributed inference control client" << endl;
    distributed_inference_control_client = new DistributedInferenceControlAgentNode(
        distributed_inference_control_node_id, distributed_inference_control_node_server_id);
    cout << "Starting inference agent thread" << endl;
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
                    if (answer.front() == PROOF_OF_IMPLICATION_OR_EQUIVALENCE) {
                        cout << "Received proof of implication or equivalence" << endl;
                        shared_ptr<ProofOfImplicationOrEquivalence> proof_of_implication_or_equivalence =
                            make_shared<ProofOfImplicationOrEquivalence>(
                                answer[1], answer[2], stoi(answer[3]));
                        iterator_link_creation_request_map[""] =
                            proof_of_implication_or_equivalence;  // iterator id
                        send_link_creation_request(proof_of_implication_or_equivalence);

                    } else if (answer.front() == PROOF_OF_IMPLICATION) {
                        Utils::error("Proof of implication is not supported yet");
                    } else if (answer.front() == PROOF_OF_EQUIVALENCE) {
                        Utils::error("Proof of equivalence is not supported yet");
                    }
                } else {
                    Utils::error("Invalid inference request");
                }
            } catch (const std::exception& e) {
                cout << "Error: " << e.what() << endl;
            }
            Utils::sleep();
        }
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
    auto query = inference_request->query();
    vector<string> link_creation_request;
    for (auto& token : query) {
        link_creation_request.push_back(token);
    }
    if (inference_request->get_type() == PROOF_OF_IMPLICATION_OR_EQUIVALENCE) {
        auto patterns_link_template =
            dynamic_cast<ProofOfImplicationOrEquivalence&>(*inference_request.get()).patterns_link_template();
        for (auto& token : patterns_link_template) {
            link_creation_request.push_back(token);
        }
    }

    link_creation_node_client->send_message(link_creation_request);
}

void InferenceAgent::send_stop_link_creation_request(InferenceRequest& inference_request) {}

void InferenceAgent::send_distributed_inference_control_request(const string& client_node_id) {}

vector<string> InferenceAgent::get_link_creation_request() { return vector<string>(); }

void InferenceAgent::parse_config(const string& config_path) {
    cout << "Parsing config file: " << config_path << endl;
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
                if (key == "inference_node_id") {
                    this->inference_node_id = value;
                } else if (key == "das_client_id") {
                    this->das_client_id = value;
                } else if (key == "das_server_id") {
                    this->das_server_id = value;
                } else if (key == "distributed_inference_control_node_id") {
                    this->distributed_inference_control_node_id = value;
                } else if (key == "distributed_inference_control_node_server_id") {
                    this->distributed_inference_control_node_server_id = value;
                } else if (key == "link_creation_agent_server_id") {
                    this->link_creation_agent_server_id = value;
                } else if (key == "link_creation_agent_client_id") {
                    this->link_creation_agent_client_id = value;
                }
            }
        }
    }
    file.close();
}
