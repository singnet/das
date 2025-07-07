#include <signal.h>

#include <cstring>
#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "LinkCreationRequestProxy.h"
#include "ServiceBusSingleton.h"
using namespace link_creation_agent;
using namespace std;
using namespace service_bus;
using namespace atomdb;

void ctrl_c_handler(int) {
    cout << "Stopping client..." << endl;
    cout << "Done." << endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    string help = R""""(
    Usage: link_creation_agent client_address server_address <start_port:end_port> REQUEST+
    Supported args:
        client_address: The address of the client to connect to, in the form "host:port"
        server_address: The address of the server to connect to, in the form "host:port"
        <start_port:end_port>: The lower and upper bound for the port numbers to be used by the command proxy
        REQUEST+: A list of tokens to be sent to the server
    Requests must be in the following format:
    QUERY, LINK_TEMPLATE, MAX_RESULTS, REPEAT, CONTEXT, UPDATE_ATTENTION_BROKER
    MAX_RESULTS and REPEAT are optional, the default value for MAX_RESULTS is 1000 and for REPEAT is 1
    )"""";

    if ((argc < 5)) {
        cerr << help << endl;
        exit(1);
    }
    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    string client_id = string(argv[1]);
    string server_id = string(argv[2]);
    auto ports_range = Utils::parse_ports_range(argv[3]);

    vector<string> request;
    for (int i = 4; i < argc; i++) {
        request.push_back(argv[i]);
    }
    AtomDBSingleton::init();
    ServiceBusSingleton::init(client_id, server_id, ports_range.first, ports_range.second);
    auto proxy = make_shared<LinkCreationRequestProxy>(request);
    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    return 0;
}
