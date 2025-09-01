#include <cmath>
#include <cstdlib>

#include "HebbianNetwork.h"
#include "HebbianNetworkUpdater.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"
#include "common.pb.h"
#include "expression_hasher.h"
#include "gtest/gtest.h"
#include "test_utils.h"

using namespace attention_broker;

TEST(HebbianNetworkUpdater, correlation) {
    string* handles = build_handle_space(6);
    HebbianNetwork* network = new HebbianNetwork();
    dasproto::HandleList* request;
    ExactCountHebbianUpdater* updater = (ExactCountHebbianUpdater*) HebbianNetworkUpdater::factory(
        HebbianNetworkUpdaterType::EXACT_COUNT);

    request = new dasproto::HandleList();
    request->set_hebbian_network((unsigned long) network);
    request->add_list(handles[0]);
    request->add_list(handles[1]);
    request->add_list(handles[2]);
    request->add_list(handles[3]);
    updater->correlation(request);

    EXPECT_TRUE(network->get_node_count(handles[0]) == 1);
    EXPECT_TRUE(network->get_node_count(handles[1]) == 1);
    EXPECT_TRUE(network->get_node_count(handles[2]) == 1);
    EXPECT_TRUE(network->get_node_count(handles[3]) == 1);
    EXPECT_TRUE(network->get_node_count(handles[4]) == 0);
    EXPECT_TRUE(network->get_node_count(handles[5]) == 0);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[0], handles[1]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[0], handles[2]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[0], handles[3]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[1], handles[2]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[1], handles[3]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[2], handles[3]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[1], handles[2]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[1], handles[4]) == 0);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[1], handles[5]) == 0);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[2], handles[4]) == 0);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[2], handles[5]) == 0);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[4], handles[5]) == 0);

    request = new dasproto::HandleList();
    request->set_hebbian_network((unsigned long) network);
    request->add_list(handles[1]);
    request->add_list(handles[2]);
    request->add_list(handles[4]);
    request->add_list(handles[5]);
    updater->correlation(request);

    EXPECT_TRUE(network->get_node_count(handles[0]) == 1);
    EXPECT_TRUE(network->get_node_count(handles[1]) == 2);
    EXPECT_TRUE(network->get_node_count(handles[2]) == 2);
    EXPECT_TRUE(network->get_node_count(handles[3]) == 1);
    EXPECT_TRUE(network->get_node_count(handles[4]) == 1);
    EXPECT_TRUE(network->get_node_count(handles[5]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[0], handles[1]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[0], handles[2]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[0], handles[3]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[1], handles[2]) == 2);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[1], handles[3]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[2], handles[3]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[1], handles[2]) == 2);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[1], handles[4]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[1], handles[5]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[2], handles[4]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[2], handles[5]) == 1);
    EXPECT_TRUE(network->get_asymmetric_edge_count(handles[4], handles[5]) == 1);
}

TEST(HebbianNetworkUpdater, determiners) {
    string* handles = build_handle_space(5);
    HebbianNetwork* network = new HebbianNetwork();
    dasproto::HandleList* request;
    ExactCountHebbianUpdater* updater = (ExactCountHebbianUpdater*) HebbianNetworkUpdater::factory(
        HebbianNetworkUpdaterType::EXACT_COUNT);

    request = new dasproto::HandleList();
    request->set_hebbian_network((unsigned long) network);
    request->add_list(handles[0]);
    request->add_list(handles[1]);
    updater->determiners(*request, network);
    delete request;

    request = new dasproto::HandleList();
    request->set_hebbian_network((unsigned long) network);
    request->add_list(handles[0]);
    request->add_list(handles[3]);
    updater->determiners(*request, network);
    delete request;

    request = new dasproto::HandleList();
    request->set_hebbian_network((unsigned long) network);
    request->add_list(handles[1]);
    request->add_list(handles[2]);
    updater->determiners(*request, network);
    delete request;

    request = new dasproto::HandleList();
    request->set_hebbian_network((unsigned long) network);
    request->add_list(handles[3]);
    request->add_list(handles[4]);
    updater->determiners(*request, network);
    delete request;

    network->lookup_node(handles[0])->importance = 0.1;
    network->lookup_node(handles[1])->importance = 0.2;
    network->lookup_node(handles[2])->importance = 0.3;
    network->lookup_node(handles[3])->importance = 0.15;
    network->lookup_node(handles[4])->importance = 0.05;

    EXPECT_TRUE(double_equals(network->get_node_importance(handles[0]), 0.2));
    EXPECT_TRUE(double_equals(network->get_node_importance(handles[1]), 0.3));
    EXPECT_TRUE(double_equals(network->get_node_importance(handles[2]), 0.3));
    EXPECT_TRUE(double_equals(network->get_node_importance(handles[3]), 0.15));
    EXPECT_TRUE(double_equals(network->get_node_importance(handles[4]), 0.05));
}
