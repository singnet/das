#include <grpcpp/grpcpp.h>

#include "AttentionBrokerClient.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"

using namespace attention_broker;
using namespace commons;

mutex AttentionBrokerClient::api_mutex;
string AttentionBrokerClient::SERVER_ADDRESS = DEFAULT_ATTENTION_BROKER_ADDRESS;
unsigned int AttentionBrokerClient::MAX_GET_IMPORTANCE_BUNDLE_SIZE = 100000;

// -------------------------------------------------------------------------------------------------
// Public methods

void AttentionBrokerClient::set_server_address(const string& ip_port) {
    SERVER_ADDRESS = ip_port;
}

void AttentionBrokerClient::correlate(const set<string>& handles, const string& context) {

    dasproto::HandleList handle_list;  // GRPC command parameter
    dasproto::Ack ack;                 // GRPC command return
    auto stub = dasproto::AttentionBroker::NewStub(grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));

    handle_list.set_context(context);
    for (string handle : handles) {
        handle_list.add_list(handle);
    }
    if (handle_list.list_size() > 0) {
        LOG_INFO("Calling AttentionBroker GRPC. Correlating " << handle_list.list_size() << " handles");
        stub->correlate(new grpc::ClientContext(), handle_list, &ack);
        if (ack.msg() != "CORRELATE") {
            Utils::error("Failed GRPC command: AttentionBroker::correlate()");
        }
    } else {
        LOG_INFO("No handles to correlate");
    }
}

void AttentionBrokerClient::stimulate(const map<string, unsigned int>& handle_map, const string& context) {

    dasproto::HandleCount handle_count;  // GRPC command parameter
    dasproto::Ack ack;                   // GRPC command return
    auto stub = dasproto::AttentionBroker::NewStub(grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));

    handle_count.set_context(context);
    unsigned int sum;
    for (auto pair : handle_map) {
        (*handle_count.mutable_map())[pair.first] = pair.second;
        sum += pair.second;
    }
    (*handle_count.mutable_map())["SUM"] = sum;
    LOG_INFO("Calling AttentionBroker GRPC. Stimulating " << handle_count.mutable_map()->size() << " handles");
    stub->stimulate(new grpc::ClientContext(), handle_count, &ack);
    if (ack.msg() != "STIMULATE") {
        Utils::error("Failed GRPC command: AttentionBroker::stimulate()");
    }
}

void AttentionBrokerClient::get_importance(const vector<string>& handles, const string& context, vector<float>& importances) {
    unsigned int pending_count = handles.size();
    unsigned int cursor = 0;
    unsigned int bundle_count = 0;
    dasproto::HandleList handle_list;
    dasproto::ImportanceList importance_list;
    while (pending_count > 0) {
        handle_list.set_context(context);
        while ((pending_count > 0) && (bundle_count < MAX_GET_IMPORTANCE_BUNDLE_SIZE)) {
            handle_list.add_list(handles[cursor++]);
            pending_count--;
            bundle_count++;
        }
        auto stub = dasproto::AttentionBroker::NewStub(grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));
        LOG_INFO("Querying AttentionBroker for importance of " << handle_list.list_size() << " atoms.");
        stub->get_importance(new grpc::ClientContext(), handle_list, &importance_list);
        for (unsigned int i = 0; i < bundle_count; i++) {
            importances.push_back(importance_list.list(i));
        }
        bundle_count = 0;
        handle_list.clear_list();
        importance_list.clear_list();
    }
}
