#include <signal.h>

#include <cstring>
#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "AtomSpace.h"
#include "AttentionBrokerClient.h"
#include "InferenceProxy.h"
#include "Logger.h"
#include "ServiceBusSingleton.h"

using namespace inference_agent;
using namespace service_bus;
using namespace atomdb;
using namespace std;
using namespace atom_space;
using namespace attention_broker;

// // clang-format off
// vector<string> context_query = {
//     "LINK_TEMPLATE", "EXPRESSION", "3",
//         "NODE", "SYMBOL", "Evaluation",
//         "VARIABLE", "V2",
//         "VARIABLE", "V3",
// };
// // clang-format on

// clang-format off
   vector<string> context_query = {
        "OR", "3",
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "Evaluation",
            "VARIABLE", "V2",
            "VARIABLE", "V3",
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "Contains",
            "VARIABLE", "V2",
            "VARIABLE", "V3",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "Concept",
            "VARIABLE", "V2",

    };
// clang-format on

void ctrl_c_handler(int) {
    std::cout << "Stopping client..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}
string query_answer_report(shared_ptr<QueryAnswer> answer, bool show_metta = false) {
    auto db = AtomDBSingleton::get_instance();
    string report = "QueryAnswer: ";
    report += to_string(answer->handles.size()) + " handles, ";
    report += to_string(answer->assignment.table.size()) + " assignments, ";
    report +=
        "importance: " + to_string(answer->importance) + ", strength: " + to_string(answer->strength);
    report += "\n\tHandles: ";
    for (const auto& handle : answer->handles) {
        report += "\t\t" +
                  (show_metta ? db->get_atom(handle)->metta_representation(*db.get()) : handle) + ", ";
    }
    report += "\n\tAssignments: ";
    for (const auto& [key, value] : answer->assignment.table) {
        report += "\t\t" + key + " -> " +
                  (show_metta ? db->get_atom(value)->metta_representation(*db.get()) : value) + ", ";
    }
    return report;
}

int main(int argc, char* argv[]) {
    string help = R""""(
    Usage: inference_agent client_address server_address <start_port:end_port> timeout max_query_answers number_of_iterations run_full_evaluation REQUEST+
    Supported args:
        client_address: The address of the client to connect to, in the form "host:port"
        server_address: The address of the server to connect to, in the form "host:port"
        <start_port:end_port>: The lower and upper bound for the port numbers to be used by the command proxy
        timeout: Maximum time in seconds to wait for each inference iteration process
        max_query_answers: Maximum number of query answers to process
        number_of_iterations: Number of times to repeat the inference process
        update_attention_broker: Whether to update the attention broker or not
        run_full_evaluation: Whether to run a full evaluation of the query
        REQUEST+: A list of tokens to be sent to the server
    Requests must be in the following format:
        PROOF_OF_IMPLICATION <handle1> <handle2> <max proof length>
        PROOF_OF_EQUIVALENCE <handle1> <handle2> <max proof length>
    )"""";

    if (argc < 9) {
        cerr << help << endl;
        exit(1);
    }

    string client_id = string(argv[1]);
    string server_id = string(argv[2]);
    auto [start_port, end_port] = Utils::parse_ports_range(argv[3]);
    string timeout_str = string(argv[4]);
    string max_query_answers_str = string(argv[5]);
    string number_of_iterations_str = string(argv[6]);
    string update_attention_broker_str = string(argv[7]);
    string run_full_evaluation_str = string(argv[8]);
    string context_tag = string(argv[argc - 1]);

    // Initialize the AtomDB singleton

    AtomDBSingleton::init();
    ServiceBusSingleton::init(client_id, server_id, start_port, end_port);
    float RENT_RATE = 0.25;
    float SPREADING_RATE_LOWERBOUND = 0.9;
    float SPREADING_RATE_UPPERBOUND = 0.9;
    AttentionBrokerClient::set_parameters(
        RENT_RATE, SPREADING_RATE_LOWERBOUND, SPREADING_RATE_UPPERBOUND);
    LOG_INFO("Setting up context");
    AtomSpace atom_space;
    QueryAnswerElement target2("V2");
    QueryAnswerElement target3("V3");
    QueryAnswerElement toplevel_link(0);
    auto context_obj = atom_space.create_context(
        context_tag, context_query, {{toplevel_link, target2}, {toplevel_link, target3}}, {});
    string context = context_obj->get_key();
    LOG_INFO("Context " + context + " is ready");
    // string context = context_tag;

    vector<string> request;
    for (int i = 9; i < argc - 1; i++) {
        request.push_back(argv[i]);
    }
    request.push_back(context);
    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    cout << ":: ::Starting inference agent:: ::" << endl;

    auto proxy = make_shared<InferenceProxy>(request);
    proxy->parameters[InferenceProxy::INFERENCE_REQUEST_TIMEOUT] = (unsigned int) stoi(timeout_str);
    proxy->parameters[InferenceProxy::MAX_ANSWERS] = (unsigned int) stoi(max_query_answers_str);
    proxy->parameters[InferenceProxy::ATTENTION_UPDATE_FLAG] =
        (update_attention_broker_str == "true" || update_attention_broker_str == "1");
    proxy->parameters[InferenceProxy::REPEAT_COUNT] = (unsigned int) stoi(number_of_iterations_str);
    auto client = ServiceBusSingleton::get_instance();

    client->issue_bus_command(proxy);
    int count = 1;
    shared_ptr<QueryAnswer> query_answer;
    while (!proxy->finished()) {
        if ((query_answer = proxy->pop()) == NULL) {
            Utils::sleep();
        } else {
            // cout << count << ": " << query_answer->to_string() << endl;
            cout << query_answer_report(query_answer, true) << endl;
        }
        count++;
    }
    if (count == 1) {
        cout << "No match for query" << endl;
    }
    cout << "Inference agent finished" << endl;
    return 0;
}