#include <signal.h>

#include <cstring>
#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "AttentionBrokerClient.h"
#include "LinkCreationRequestProcessor.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

using namespace std;
using namespace link_creation_agent;
using namespace atomdb;
using namespace service_bus;
using namespace attention_broker;

void ctrl_c_handler(int) {
    std::cout << "Stopping server..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

/**
 * @brief Main function
 * Reads the config file and starts the DAS NODE client/server
 * @param argc Number of arguments
 * @param argv Arguments
 * @returns Returns 0 if the program runs successfully
 */
int main(int argc, char* argv[]) {
    string help = R""""(
    Usage: link_creation_agent server_address peer_address <start_port:end_port> [request_interval] [thread_count] [default_timeout] [buffer_file] [metta_file_path] [save_links_to_metta_file] [save_links_to_db] [AB_ip:AB_port]
    Suported args:
        server_address: The address of the server to connect to, in the form "host:port"
        peer_address: The address of the peer to connect to, in the form "host:port"
        <start_port:end_port> The lower and upper bound for the port numbers to be used by the command proxy
        request_interval: The interval in seconds to send requests (default: 1)
        thread_count: The number of threads to process requests (default: 1)
        default_timeout: The timeout in seconds for query requests (default: 10)
        buffer_file: The path to the requests buffer file (default: "requests_buffer.bin")
        metta_file_path: The path to the metta file (default: "./")
        save_links_to_metta_file: Whether to save links to the metta file (default: true)
        save_links_to_db: Whether to save links to the database (default: false)
        [AB_ip:AB_port] The address of the Attention Broker to connect to, in the form "host:port". 

    Requests must be in the following format:
    QUERY, LINK_TEMPLATE, MAX_ANSWERS, REPEAT
    MAX_ANSWERS and REPEAT are optional, the default value for MAX_ANSWERS is 1000 and for REPEAT is 1
    )"""";

    if (argc < 4) {
        cerr << help << endl;
        for (auto arg = 0; arg < argc; arg++) {
            cerr << "arg[" << arg << "] = " << argv[arg] << endl;
        }
        exit(1);
    }
    string server_address = argv[1];
    string peer_address = argv[2];
    auto [start_port, end_port] = Utils::parse_ports_range(argv[3]);
    int request_interval = argc > 4 ? Utils::string_to_int(argv[4]) : 60;
    int thread_count = argc > 5 ? Utils::string_to_int(argv[5]) : 1;
    int default_timeout = argc > 6 ? Utils::string_to_int(argv[6]) : 50;
    string buffer_file = argc > 7 ? argv[7] : "requests_buffer.bin";
    string metta_file_path = argc > 8 ? argv[8] : "./";
    bool save_links_to_metta_file = argc > 9 && (string(argv[9]) == string("true") ||
                                                 string(argv[9]) == "1" || string(argv[9]) == "yes");
    bool save_links_to_db = argc > 10 && (string(argv[10]) == string("true") ||
                                          string(argv[10]) == "1" || string(argv[10]) == "yes");

    if (argc == 12) {
        attention_broker::AttentionBrokerClient::set_server_address(argv[11]);
    }

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    AtomDBSingleton::init();
    ServiceBusSingleton::init(server_address, peer_address, start_port, end_port);
    shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
    service_bus->register_processor(make_shared<LinkCreationRequestProcessor>(request_interval,
                                                                              thread_count,
                                                                              default_timeout,
                                                                              buffer_file,
                                                                              metta_file_path,
                                                                              save_links_to_metta_file,
                                                                              save_links_to_db));

    do {
        Utils::sleep(1000);
    } while (true);
    return 0;
}
