#include "AttentionBrokerClient.h"

#include <grpcpp/grpcpp.h>

#include "Utils.h"

#define LOG_LEVEL LOCAL_DEBUG_LEVEL
#include "Logger.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"

using namespace attention_broker;
using namespace commons;

mutex AttentionBrokerClient::api_mutex;
string AttentionBrokerClient::SERVER_ADDRESS = DEFAULT_ATTENTION_BROKER_ADDRESS;
unsigned int AttentionBrokerClient::MAX_GET_IMPORTANCE_BUNDLE_SIZE = 100000;
unsigned int AttentionBrokerClient::MAX_SET_DETERMINERS_HANDLE_COUNT = 100000;

// -------------------------------------------------------------------------------------------------
// Public methods

void AttentionBrokerClient::set_server_address(const string& ip_port) { SERVER_ADDRESS = ip_port; }

void AttentionBrokerClient::correlate(const set<string>& handles, const string& context) {
    dasproto::HandleList handle_list;  // GRPC command parameter
    dasproto::Ack ack;                 // GRPC command return
    auto stub = dasproto::AttentionBroker::NewStub(
        grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));

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

void AttentionBrokerClient::asymmetric_correlate(const vector<string>& handles, const string& context) {
    dasproto::HandleList handle_list;  // GRPC command parameter
    dasproto::Ack ack;                 // GRPC command return
    auto stub = dasproto::AttentionBroker::NewStub(
        grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));

    handle_list.set_context(context);
    for (string handle : handles) {
        handle_list.add_list(handle);
    }
    if (handle_list.list_size() > 0) {
        LOG_INFO("Calling AttentionBroker GRPC. Correlating (asymmetric) " << handle_list.list_size()
                                                                           << " handles");
        stub->asymmetric_correlate(new grpc::ClientContext(), handle_list, &ack);
        if (ack.msg() != "ASYMMETRIC_CORRELATE") {
            Utils::error("Failed GRPC command: AttentionBroker::asymmetric_correlate()");
        }
    } else {
        LOG_INFO("No handles to correlate (asymmetric)");
    }
}

void AttentionBrokerClient::stimulate(const map<string, unsigned int>& handle_map,
                                      const string& context) {
    dasproto::HandleCount handle_count;  // GRPC command parameter
    dasproto::Ack ack;                   // GRPC command return
    auto stub = dasproto::AttentionBroker::NewStub(
        grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));

    handle_count.set_context(context);
    unsigned int sum = 0;
    for (auto pair : handle_map) {
        (*handle_count.mutable_map())[pair.first] = pair.second;
        sum += pair.second;
    }
    (*handle_count.mutable_map())["SUM"] = sum;
    LOG_INFO("Calling AttentionBroker GRPC. Stimulating " << handle_count.mutable_map()->size() - 1
                                                          << " handles");
    stub->stimulate(new grpc::ClientContext(), handle_count, &ack);
    if (ack.msg() != "STIMULATE") {
        Utils::error("Failed GRPC command: AttentionBroker::stimulate()");
    }
}

void AttentionBrokerClient::set_determiners(const vector<vector<string>>& handle_lists,
                                            const string& context) {
    dasproto::HandleListList request;  // GRPC command parameter
    dasproto::Ack ack;                 // GRPC command return
    auto stub = dasproto::AttentionBroker::NewStub(
        grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));

    unsigned int pending_count = handle_lists.size();
    unsigned int cursor = 0;
    unsigned int handle_count = 0;

    while (pending_count > 0) {
        request.set_context(context);
        unsigned int inner_cursor = 0;
        while ((pending_count > 0) && (handle_count < MAX_SET_DETERMINERS_HANDLE_COUNT)) {
            request.add_list();
            for (auto handle : handle_lists[cursor]) {
                request.mutable_list(inner_cursor)->add_list(handle);
            }
            inner_cursor++;
            pending_count--;
            handle_count += handle_lists[cursor].size();
            cursor++;
        }
        LOG_INFO("Calling AttentionBroker GRPC. Setting determiners for " << request.list_size()
                                                                          << " handles");
        stub->set_determiners(new grpc::ClientContext(), request, &ack);
        if (ack.msg() != "SET_DETERMINERS") {
            Utils::error("Failed GRPC command: AttentionBroker::set_determiners()");
        }
        request.clear_list();
        handle_count = 0;
    }
}

void AttentionBrokerClient::get_importance(const vector<string>& handles,
                                           const string& context,
                                           vector<float>& importances) {
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
        auto stub = dasproto::AttentionBroker::NewStub(
            grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));
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

void AttentionBrokerClient::set_parameters(float rent_rate,
                                           float spreading_rate_lowerbound,
                                           float spreading_rate_upperbound) {
    dasproto::Parameters request;  // GRPC command parameter
    dasproto::Ack ack;             // GRPC command return

    request.set_rent_rate(rent_rate);
    request.set_spreading_rate_lowerbound(spreading_rate_lowerbound);
    request.set_spreading_rate_upperbound(spreading_rate_upperbound);

    auto stub = dasproto::AttentionBroker::NewStub(
        grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));

    LOG_INFO("Calling AttentionBroker GRPC. Setting dynamics parameters. RENT_RATE: "
             << request.rent_rate()
             << " SPREADING_RATE_LOWERBOUND: " << request.spreading_rate_lowerbound()
             << " SPREADING_RATE_UPPERBOUND: " << request.spreading_rate_upperbound());
    stub->set_parameters(new grpc::ClientContext(), request, &ack);
    if (ack.msg() != "SET_PARAMETERS") {
        Utils::error("Failed GRPC command: AttentionBroker::set_parameters()");
    }
}

bool AttentionBrokerClient::health_check(bool throw_on_error) {
    dasproto::Empty request;  // GRPC command parameter
    dasproto::Ack ack;        // GRPC command return

    auto stub = dasproto::AttentionBroker::NewStub(
        grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));

    LOG_INFO("Calling AttentionBroker GRPC. Ping");
    stub->ping(new grpc::ClientContext(), request, &ack);
    if (ack.msg() == "PING") {
        return true;
    } else {
        Utils::error("Attention broker is not responding on " + SERVER_ADDRESS, throw_on_error);
        return false;
    }
}

void AttentionBrokerClient::save_context(const string& context, const string& file_name) {
    dasproto::ContextPersistence request;
    dasproto::Ack ack;

    request.set_context(context);
    request.set_file_name(file_name);

    auto stub = dasproto::AttentionBroker::NewStub(
        grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));

    LOG_INFO("Calling AttentionBroker GRPC. Saving context contents into a file. Context: " + context +
             " File name: " + file_name);
    stub->drop_and_load_context(new grpc::ClientContext(), request, &ack);
    if (ack.msg() != "SAVE_CONTEXT") {
        Utils::error("Failed GRPC command: AttentionBroker::save_context()");
    }
}

void AttentionBrokerClient::drop_and_load_context(const string& context, const string& file_name) {
    dasproto::ContextPersistence request;
    dasproto::Ack ack;

    request.set_context(context);
    request.set_file_name(file_name);

    auto stub = dasproto::AttentionBroker::NewStub(
        grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));

    LOG_INFO(
        "Calling AttentionBroker GRPC. Dropping context info and loading contents from file. Context: " +
        context + " File name: " + file_name);
    stub->drop_and_load_context(new grpc::ClientContext(), request, &ack);
    if (ack.msg() != "DROP_AND_LOAD_CONTEXT") {
        Utils::error("Failed GRPC command: AttentionBroker::drop_and_load_context()");
    }
}
