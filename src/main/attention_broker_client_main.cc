#include <vector>
#include <iostream>
#include <grpcpp/grpcpp.h>
#include "AtomDBSingleton.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"

using namespace std;
using namespace atomdb;

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Attention broker client" << endl;
        cerr << "Usage: " << argv[0] << " host:port <command> arg+" << endl;
        return 1;
    }

    string attention_broker_address = string(argv[1]);
    string command = string(argv[2]);
    vector<string> args;
    for (int i = 3; i < argc; i++) {
        args.push_back(string(argv[i]));
    }
    auto stub = dasproto::AttentionBroker::NewStub(grpc::CreateChannel(attention_broker_address, grpc::InsecureChannelCredentials()));
    AtomDBSingleton::init();
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();

    if (command == "importance") {
        dasproto::HandleList handle_list;
        dasproto::ImportanceList importance_list;
        for (string handle: args) {
            handle_list.add_list(handle);
        }
        stub->get_importance(new grpc::ClientContext(), handle_list, &importance_list);
        for (unsigned int i = 0; i < args.size(); i++) {
            auto atom = db->get_atom(handle_list.list(i));
            cout << handle_list.list(i) << " " << atom->metta_representation(*(db.get())) << ": " << std::to_string(importance_list.list(i)) << endl;
        }
    } else {
        cerr << "Invalid command: " << command << endl;
        return 1;
    }
    return 0;
}
