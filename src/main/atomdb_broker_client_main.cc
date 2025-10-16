#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"
#include "AtomDBBrokerProxy.h"
#include "Properties.h"

using namespace std;
using namespace service_bus;
using namespace atoms;
using namespace atomdb;
using namespace atomdb_broker;


int main(int argc, char* argv[]) {
    if (argc < 6) {
        cerr << "Usage: " << argv[0]
             << " <client_ip:client_port> <server_ip:server_port> <start_port:end_port> <action> <tokens+> "
             << endl;
        exit(1);
    }

    string client_id = string(argv[1]);
    string server_id = string(argv[2]);
    auto ports_range = Utils::parse_ports_range(argv[3]);
    string action = string(argv[4]);
    vector<string> tokens;
    for (int i = 5; i < argc; i++) {
        tokens.push_back(argv[i]);
    }

    AtomDBSingleton::init();
    ServiceBusSingleton::init(client_id, server_id, ports_range.first, ports_range.second);
    auto proxy = make_shared<AtomDBBrokerProxy>(action);
    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);

    if (action == AtomDBBrokerProxy::ADD_ATOMS) {
        cout << "[[ Client A ]]" << endl;
        for (auto a : tokens) {
            cout << "--> " << a << endl;
        }
        auto atoms = proxy->build_atoms_from_tokens(tokens);
        cout << "[[ Client B ]]" << endl;
        auto ret = proxy->add_atoms(atoms);
        cout << "[[ Client C ]]" << endl;
        cout << "Response: " << ret[0] << endl;
    }


    // ------ prints

    // Properties attrs;
    // attrs["is_literal"] = true;

    // auto n = new atoms::Node("Symbol", "human", attrs);
    // auto n2 = new atoms::Node("TypeFake", "nameFake");

    // Properties attrs2;
    // attrs2["strengh"] = 0.95;
    // attrs2["confidence"] = 0.23;

    // auto l = new atoms::Link("Expression", {"target1", "target2", "target3"}, true, attrs2);

    // vector<string> t1;
    // n->tokenize(t1);

    // cout << "----BEFORE------" << endl;
    // cout << "Type2: " << n2->type << endl;
    // cout << "Name2: " << n2->name << endl;
    // n2->untokenize(t1);
    // cout << "-----AFTER-----" << endl;
    // cout << "Type2: " << n2->type << endl;
    // cout << "Name2: " << n2->name << endl;

    // vector<string> t2;
    // l->tokenize(t2);

    // string t1_out;
    // for (auto t : t1) {
    //     t1_out += t;
    //     t1_out += " ";
    // }
    // string t2_out;
    // for (auto tx : t2) {
    //     t2_out += tx;
    //     t2_out += " ";
    // }

    // cout << "Token node: " << t1_out << endl;
    // cout << "----------" << endl;
    // cout << "Type: " << n->type << endl;
    // cout << "Name: " << n->name << endl;
    
    // cout << "----------" << endl;
    // cout << "Type2: " << n2->type << endl;
    // cout << "Name2: " << n2->name << endl;
    
    // cout << "Token link: " << t2_out << endl;

    return 0;
}
