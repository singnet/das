#include <iostream>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <signal.h>

#include "common.pb.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"

#include "AttentionBrokerServer.h"

//attention_broker_server::AttentionBrokerServer service;

/*
void ctrl_c_handler(int) {
    std::cout << "Stopping AttentionBrokerServer..." << std::endl;
    service.graceful_shutdown();
    std::cout << "Done." << std::endl;
    exit(0);
}
*/

void run_server(unsigned int port) {
    attention_broker_server::AttentionBrokerServer service;
    std::string server_address = "localhost:" + to_string(port);
    //grpc::EnableDefaultHealthCheckService(true);
    //grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "AttentionBroker server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Attention broker" << endl;
        cerr << "Usage: " << argv[0] << " PORT" << endl;
        exit(1);
    }
    unsigned int port = stoi(argv[1]);
    //signal(SIGINT, &ctrl_c_handler);
    run_server(port);
    return 0;
}
