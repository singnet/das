#include <cstdlib>
#include <cmath>

#include "gtest/gtest.h"
#include "common.pb.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"
#include "test_utils.h"
#include "expression_hasher.h"
#include "HebbianNetwork.h"
#include "HebbianNetworkUpdater.h"

using namespace attention_broker_server;

TEST(HebbianNetworkUpdater, correlation) {
    string *handles = build_handle_space(6);
    HebbianNetwork *network = new HebbianNetwork();
    dasproto::HandleList *request;
    ExactCountHebbianUpdater *updater = \
        (ExactCountHebbianUpdater *) HebbianNetworkUpdater::factory(HebbianNetworkUpdaterType::EXACT_COUNT);

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
