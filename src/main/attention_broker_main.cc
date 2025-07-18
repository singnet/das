#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <signal.h>

#include <iostream>
#include <string>

#include "AttentionBrokerServer.h"
#include "Logger.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"
#include "common.pb.h"

attention_broker_server::AttentionBrokerServer service;

void ctrl_c_handler(int) {
    LOG_INFO("Stopping AttentionBrokerServer...");
    service.graceful_shutdown();
    LOG_INFO("Done.");
    exit(0);
}

void run_server(unsigned int port) {
    std::string server_address = "0.0.0.0:" + to_string(port);
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    LOG_INFO("AttentionBroker server listening on " + server_address);
    server->Wait();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Attention broker" << endl;
        cerr << "Usage: " << argv[0] << " PORT" << endl;
        exit(1);
    }
    unsigned int port = stoi(argv[1]);
    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    run_server(port);
    return 0;
}
