#include <signal.h>

#include "inference_agent.h"
using namespace inference_agent;
using namespace std;

void ctrl_c_handler(int) {
    std::cout << "Stopping server..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    string help = R""""(
    Usage: inference_agent --config_file <path>
    Suported args:
    --config_file   path to config file
    --help          print this message
    )"""";

    if (argc < 2) {
        cerr << help << endl;
        exit(1);
    }
    signal(SIGINT, &ctrl_c_handler);
    string config_path = argv[2];
    cout << "Starting inference agent" << endl;
    auto server = new InferenceAgent(config_path);
    server->run();
    return 0;
}