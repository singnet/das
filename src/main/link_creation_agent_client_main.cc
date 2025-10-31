#include <signal.h>

#include <cstring>
#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "LinkCreationRequestProxy.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"
using namespace link_creation_agent;
using namespace std;
using namespace service_bus;
using namespace atomdb;

bool running = true;

void ctrl_c_handler(int) {
    cout << "Stopping client..." << endl;
    running = false;
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
        <max_answers>: Maximum number of results to return (default: 1000)
        <repeat_count_count>: Number of times to repeat_count the request (set 0 for infinite)
        <attention_update_flag>: Whether to update the attention broker (true/false)
        <positive_importance_flag>: Whether to set the importance flag (true/false)
        <use_metta_as_query_tokens>: Whether to use MeTTa expression as query tokens (true/false)
        <context>: Context for the link creation request
        REQUEST+/MeTTa: A list of tokens to be sent to the server or MeTTa expression
    Requests must be in the following format:
    QUERY+, ( LINK_CREATE+ | PROOF_OF_EQUIVALENCE | PROOF_OF_IMPLICATION )
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
    int max_answers = Utils::string_to_int(string(argv[4]));
    int repeat_count = Utils::string_to_int(string(argv[5]));
    bool attention_update_flag = (string(argv[6]) == "true");
    bool positive_importance_flag = (string(argv[7]) == "true");
    bool use_metta_as_query_tokens = (string(argv[8]) == "true");
    string context = string(argv[9]);

    vector<string> request;
    for (int i = 10; i < argc; i++) {
        string token = string(argv[i]);
        if (use_metta_as_query_tokens) {
            Utils::replace_all(token, "%", "$");
        }
        request.push_back(token);
        cout << "Request token: " << token << endl;
    }
    AtomDBSingleton::init();
    ServiceBusSingleton::init(client_id, server_id, ports_range.first, ports_range.second);
    auto proxy = make_shared<LinkCreationRequestProxy>(request);
    proxy->parameters[LinkCreationRequestProxy::MAX_ANSWERS] = (unsigned int) max_answers;
    proxy->parameters[LinkCreationRequestProxy::REPEAT_COUNT] = (unsigned int) repeat_count;
    proxy->parameters[LinkCreationRequestProxy::CONTEXT] = context;
    proxy->parameters[LinkCreationRequestProxy::ATTENTION_UPDATE_FLAG] = attention_update_flag;
    proxy->parameters[LinkCreationRequestProxy::POSITIVE_IMPORTANCE_FLAG] = positive_importance_flag;
    proxy->parameters[LinkCreationRequestProxy::USE_METTA_AS_QUERY_TOKENS] = use_metta_as_query_tokens;
    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    while(!proxy->finished()) {
        Utils::sleep();
        if (!running) {
            proxy->abort();
        }
    }
    return 0;
}
