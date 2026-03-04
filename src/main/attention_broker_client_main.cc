#include <grpcpp/grpcpp.h>

#include <iostream>
#include <vector>

#include "AtomDBSingleton.h"
#include "Utils.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"

using namespace std;
using namespace atomdb;

int main(int argc, char* argv[]) {
    if (argc < 5) {
        cerr << "Attention broker client" << endl;
        cerr << "Usage: " << argv[0]
             << " host:port --atomdb-type=<morkdb|inmemorydb|remotedb> <command> arg+" << endl;
        return 1;
    }

    int command_index = 3;

    string attention_broker_address = string(argv[1]);
    string atomdb_type = string(argv[2]);
    Utils::replace_all(atomdb_type, "--atomdb-type=", "");
    if (atomdb_type == "morkdb") {
        AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::MORKDB);
    } else if (atomdb_type == "inmemorydb") {
        AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::INMEMORYDB);
    } else if (atomdb_type == "remotedb") {
        auto json_file_path = string(argv[command_index]);
        command_index++;
        if (json_file_path == "") {
            Utils::error(
                "RemoteDB config file path is required: --atomdb-type=remotedb <json_file_path>");
            return 1;
        }
        AtomDBSingleton::init(atomdb_api_types::ATOMDB_TYPE::REMOTEDB, json_file_path);
    } else {
        AtomDBSingleton::init();
    }
    string command = string(argv[command_index]);
    vector<string> args;
    for (int i = command_index + 1; i < argc; i++) {
        args.push_back(string(argv[i]));
    }
    auto stub = dasproto::AttentionBroker::NewStub(
        grpc::CreateChannel(attention_broker_address, grpc::InsecureChannelCredentials()));
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();

    if (command == "importance") {
        dasproto::HandleList handle_list;
        dasproto::ImportanceList importance_list;
        for (string handle : args) {
            handle_list.add_list(handle);
        }
        stub->get_importance(new grpc::ClientContext(), handle_list, &importance_list);
        for (unsigned int i = 0; i < args.size(); i++) {
            auto atom = db->get_atom(handle_list.list(i));
            cout << handle_list.list(i) << " " << atom->metta_representation(*(db.get())) << ": "
                 << std::to_string(importance_list.list(i)) << endl;
        }
    } else {
        cerr << "Invalid command: " << command << endl;
        return 1;
    }
    return 0;
}
