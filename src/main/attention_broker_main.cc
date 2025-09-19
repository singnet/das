#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <signal.h>

#include <iostream>
#include <string>

#include "AttentionBrokerClient.h"
#include "AttentionBrokerServer.h"
#include "Logger.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"
#include "common.pb.h"

attention_broker::AttentionBrokerServer service;

void ctrl_c_handler(int) {
    LOG_INFO("Stopping AttentionBrokerServer...");
    service.graceful_shutdown();
    LOG_INFO("Done.");
    exit(0);
}

void run_server(std::string server_address) {
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
        cerr << "Usage: " << argv[0] << " <HOSTNAME>:<PORT>" << endl;
        exit(1);
    }

    std::string server_address = string(argv[1]);

    attention_broker::AttentionBrokerClient::set_server_address(server_address);

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    run_server(server_address);
    return 0;
}
