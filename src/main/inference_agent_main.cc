#include <signal.h>

#include "AtomDBSingleton.h"
#include "FitnessFunctionRegistry.h"
#include "InferenceProcessor.h"
#include "ServiceBusSingleton.h"

using namespace inference_agent;
using namespace atomdb;
using namespace std;

void ctrl_c_handler(int) {
    std::cout << "Stopping server..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    string help = R""""(
    Usage: inference_agent server_address peer_address <start_port:end_port>
    Suported args:
        server_address: The address of the server to connect to, in the form "host:port"
        peer_address: The address of the peer to connect to, in the form "host:port"
        <start_port:end_port>: The lower and upper bound for the port numbers to be used by the command proxy
    )"""";

    if (argc < 3) {
        cerr << help << endl;
        exit(1);
    }
    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    cout << ":: ::Starting inference agent server:: ::" << endl;
    // Initialize the AtomDB singleton
    AtomDBSingleton::init();
    // Initialize the FitnessFunctionRegistry
    FitnessFunctionRegistry::initialize_statics();
    auto [start_port, end_port] = Utils::parse_ports_range(argv[3]);
    ServiceBusSingleton::init(argv[1],     // server_address
                              argv[2],     // peer_address
                              start_port,  // start_port
                              end_port     // end_port
    );
    auto service_bus = ServiceBusSingleton::get_instance();
    service_bus->register_processor(make_shared<InferenceProcessor>());

    do {
        Utils::sleep(1000);
    } while (true);

    return 0;
}
