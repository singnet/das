#include <signal.h>

#include "InferenceProcessor.h"
#include "ServiceBusSingleton.h"
using namespace inference_agent;
using namespace std;

void ctrl_c_handler(int) {
    std::cout << "Stopping server..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    string help = R""""(
    Usage: inference_agent server_address peer_address [start_port] [end_port]
    Suported args:
        server_address: The address of the server to connect to, in the form "host:port"
        peer_address: The address of the peer to connect to, in the form "host:port"
        start_port: The lower bound for the port numbers to be used by the command proxy (default: 64000)
        end_port: The upper bound for the port numbers to be used by the command proxy (default: 64999)
    )"""";

    if (argc < 2) {
        cerr << help << endl;
        exit(1);
    }
    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    cout << "Starting inference agent server" << endl;
    ServiceBusSingleton::init(
        argv[1], // server_address
        argv[2], // peer_address
        argc > 3 ? Utils::string_to_int(argv[3]) : 64000, // start_port
        argc > 4 ? Utils::string_to_int(argv[4]) : 64999 // end_port
    );
    auto service_bus = ServiceBusSingleton::get_instance();
    service_bus->register_processor(make_shared<InferenceProcessor>());

    do {
        Utils::sleep(1000);
    } while (true);

    return 0;
}