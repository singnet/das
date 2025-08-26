#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>
#include <string>

#include "AttentionBrokerServer.h"
#include "Utils.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"
#include "common.pb.h"
#include "gtest/gtest.h"
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
    dasproto::HandleListList handle_list_list;
    dasproto::Ack ack;
    dasproto::ImportanceList importance_list;
    ServerContext* context = NULL;

    service.ping(context, &empty, &ack);
    EXPECT_EQ(ack.msg(), "PING");
    service.stimulate(context, &handle_count, &ack);
    EXPECT_EQ(ack.msg(), "STIMULATE");
    service.correlate(context, &handle_list, &ack);
    EXPECT_EQ(ack.msg(), "CORRELATE");
    service.set_determiners(context, &handle_list_list, &ack);
    EXPECT_EQ(ack.msg(), "SET_DETERMINERS");
    service.get_importance(context, &handle_list, &importance_list);
    EXPECT_EQ(importance_list.list_size(), 0);
}

TEST(AttentionBrokerTest, get_importance) {
    string* handles = build_handle_space(4);

    AttentionBrokerServer service;
    dasproto::HandleList handle_list0;
    dasproto::HandleList handle_list1;
    dasproto::HandleList handle_list2;
    dasproto::HandleCount handle_count;
    dasproto::Ack ack;
    dasproto::ImportanceList importance_list1;
    dasproto::ImportanceList importance_list2;
    ServerContext* context = NULL;

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

    dasproto::HandleListList handle_list_list;
    handle_list_list.add_list();
    handle_list_list.mutable_list(0)->add_list(handles[3]);
    handle_list_list.mutable_list(0)->add_list(handles[0]);
    dasproto::ImportanceList importance_list3;
    dasproto::ImportanceList importance_list4;
    service.set_determiners(context, &handle_list_list, &ack);
    service.get_importance(context, &handle_list1, &importance_list3);
    service.get_importance(context, &handle_list2, &importance_list4);

    EXPECT_TRUE(importance_list3.list(0) > 0.4);
    EXPECT_TRUE(importance_list3.list(1) > 0.4);
    EXPECT_TRUE(importance_list4.list(0) < 0.1);
    EXPECT_TRUE(importance_list4.list(1) > 0.4);
}
