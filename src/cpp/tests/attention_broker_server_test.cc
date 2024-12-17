#include <iostream>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "gtest/gtest.h"

#include "common.pb.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"

#include "AttentionBrokerServer.h"
#include "Utils.h"
#include "test_utils.h"

using namespace attention_broker_server;
using namespace commons;

bool importance_equals(ImportanceType importance, double v2) {
    double v1 = (double) importance;
    return fabs(v2 - v1) < 0.001;
}

TEST(AttentionBrokerTest, basics) {
    
    AttentionBrokerServer service;
    dasproto::Empty empty;
    dasproto::HandleCount handle_count;
    dasproto::HandleList handle_list;
    dasproto::Ack ack;
    dasproto::ImportanceList importance_list;
    ServerContext *context = NULL;

    service.ping(context, &empty, &ack);
    EXPECT_EQ(ack.msg(), "PING");
    service.stimulate(context, &handle_count, &ack);
    EXPECT_EQ(ack.msg(), "STIMULATE");
    service.correlate(context, &handle_list, &ack);
    EXPECT_EQ(ack.msg(), "CORRELATE");
    service.get_importance(context, &handle_list, &importance_list);
    EXPECT_EQ(importance_list.list_size(), 0);
}

TEST(AttentionBrokerTest, get_importance) {
    
    string *handles = build_handle_space(4);

    AttentionBrokerServer service;
    dasproto::HandleList handle_list0;
    dasproto::HandleList handle_list1;
    dasproto::HandleList handle_list2;
    dasproto::HandleCount handle_count;
    dasproto::Ack ack;
    dasproto::ImportanceList importance_list1;
    dasproto::ImportanceList importance_list2;
    ServerContext *context = NULL;

    (*handle_count.mutable_map())[handles[0]] = 1;
    (*handle_count.mutable_map())[handles[1]] = 1;
    (*handle_count.mutable_map())["SUM"] = 2;

    handle_list0.add_list(handles[0]);
    handle_list0.add_list(handles[1]);
    handle_list0.add_list(handles[2]);
    handle_list0.add_list(handles[3]);
    handle_list1.add_list(handles[0]);
    handle_list1.add_list(handles[1]);
    handle_list2.add_list(handles[2]);
    handle_list2.add_list(handles[3]);

    service.correlate(context, &handle_list0, &ack);
    Utils::sleep(1000);
    service.stimulate(context, &handle_count, &ack);
    Utils::sleep(1000);
    service.get_importance(context, &handle_list1, &importance_list1);
    service.get_importance(context, &handle_list2, &importance_list2);

    EXPECT_TRUE(importance_list1.list(0) > 0.4);
    EXPECT_TRUE(importance_list1.list(1) > 0.4);
    EXPECT_TRUE(importance_list2.list(0) < 0.1);
    EXPECT_TRUE(importance_list2.list(1) < 0.1);
}
