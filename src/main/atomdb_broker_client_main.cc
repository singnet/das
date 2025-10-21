#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"
#include "AtomDBProxy.h"
#include "Properties.h"

using namespace std;
using namespace service_bus;
using namespace atoms;
using namespace atomdb;
using namespace atomdb_broker;


int main(int argc, char* argv[]) {
    if (argc < 6) {
        cerr << "Usage: " << argv[0]
             << " <client_ip:client_port> <peer_ip:peer_port> <start_port:end_port> <action> <tokens+> "
             << endl;
        exit(1);
    }

    string client_id = string(argv[1]);
    string peer_id = string(argv[2]);
    auto ports_range = Utils::parse_ports_range(argv[3]);
    string action = string(argv[4]);
    vector<string> tokens;
    for (int i = 5; i < argc; i++) {
        tokens.push_back(argv[i]);
    }

    AtomDBSingleton::init();
    ServiceBusSingleton::init(client_id, peer_id, ports_range.first, ports_range.second);
    auto proxy = make_shared<AtomDBProxy>();
    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    Utils::sleep(1000);

    if (action == AtomDBProxy::ADD_ATOMS) {
        auto atoms = proxy->build_atoms_from_tokens(tokens);
        vector<string> response = proxy->add_atoms(atoms);

        while (!proxy->finished()) {
            Utils::sleep();
        }
        
        if (response.empty()) {
            cout << "No answers" << endl;
        } else {
            for (auto resp : response) {
                cout << "handle: " << resp << endl;
            }
        }
    }

    return 0;
}
