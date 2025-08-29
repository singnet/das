#include <signal.h>

#include <cstring>
#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "InferenceProxy.h"
#include "ServiceBusSingleton.h"

using namespace inference_agent;
using namespace service_bus;
using namespace atomdb;
using namespace std;

void ctrl_c_handler(int) {
    std::cout << "Stopping client..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}
string query_answer_report(shared_ptr<QueryAnswer> answer, bool show_metta=false) {
    auto db = AtomDBSingleton::get_instance();
    string report = "QueryAnswer: ";
    report += to_string(answer->handles.size()) + " handles, ";
    report += to_string(answer->assignment.table.size()) + " assignments, ";
    report += "importance: " + to_string(answer->importance) + ", strength: " + to_string(answer->strength);
    report += "\n\tHandles: ";
    for (const auto& handle : answer->handles) {
        report += "\t\t" + (show_metta ? db->get_atom(handle)->metta_representation(*db.get()) : handle) + ", ";
    }
    report += "\n\tAssignments: ";
    for (const auto& [key, value] : answer->assignment.table) {
        report += "\t\t" + key + " -> " + (show_metta ? db->get_atom(value)->metta_representation(*db.get()) : value) + ", ";
    }
    return report;
}


int main(int argc, char* argv[]) {
    string help = R""""(
    Usage: inference_agent client_address server_address <start_port:end_port> REQUEST+
    Supported args:
        client_address: The address of the client to connect to, in the form "host:port"
        server_address: The address of the server to connect to, in the form "host:port"
        <start_port:end_port>: The lower and upper bound for the port numbers to be used by the command proxy
        REQUEST+: A list of tokens to be sent to the server
    Requests must be in the following format:
        PROOF_OF_IMPLICATION_OR_EQUIVALENCE <handle1> <handle2> <max proof length>
        PROOF_OF_IMPLICATION <handle1> <handle2> <max proof length>
        PROOF_OF_EQUIVALENCE <handle1> <handle2> <max proof length>

    )"""";

    if (argc < 4) {
        cerr << help << endl;
        exit(1);
    }

    string client_id = string(argv[1]);
    string server_id = string(argv[2]);
    auto [start_port, end_port] = Utils::parse_ports_range(argv[3]);

    vector<string> request;
    for (int i = 4; i < argc; i++) {
        request.push_back(argv[i]);
    }
    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    cout << ":: ::Starting inference agent:: ::" << endl;
    // Initialize the AtomDB singleton
    AtomDBSingleton::init();
    ServiceBusSingleton::init(client_id, server_id, start_port, end_port);
    auto proxy = make_shared<InferenceProxy>(request);
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