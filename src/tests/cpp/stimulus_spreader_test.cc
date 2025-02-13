#include <cstdlib>
#include <cmath>

#include "gtest/gtest.h"
#include "common.pb.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"
#include "test_utils.h"
#include "expression_hasher.h"
#include "AttentionBrokerServer.h"
#include "HebbianNetwork.h"
#include "HebbianNetworkUpdater.h"
#include "StimulusSpreader.h"

using namespace attention_broker_server;

bool importance_equals(ImportanceType importance, double v2) {
    double v1 = (double) importance;
    return fabs(v2 - v1) < 0.001;
}

TEST(TokenSpreader, distribute_wages) {

    unsigned int num_tests = 10000;
    unsigned int total_nodes = 100;

    TokenSpreader *spreader;
    ImportanceType tokens_to_spread;
    dasproto::HandleCount *request;
    TokenSpreader::StimuliData data;

    for (unsigned int i = 0; i < num_tests; i++) {
        string *handles = build_handle_space(total_nodes);
        spreader = (TokenSpreader *) StimulusSpreader::factory(StimulusSpreaderType::TOKEN);

        tokens_to_spread = 1.0;
        request = new dasproto::HandleCount();
        (*request->mutable_map())[handles[0]] = 2;
        (*request->mutable_map())[handles[1]] = 1;
        (*request->mutable_map())[handles[2]] = 2;
        (*request->mutable_map())[handles[3]] = 1;
        (*request->mutable_map())[handles[4]] = 2;
        (*request->mutable_map())["SUM"] = 8;
        data.importance_changes = new HandleTrie(HANDLE_HASH_SIZE - 1);
        spreader->distribute_wages(request, tokens_to_spread, &data);

        EXPECT_TRUE(importance_equals(((TokenSpreader::ImportanceChanges *) data.importance_changes->lookup(handles[0]))->wages, 0.250));
        EXPECT_TRUE(importance_equals(((TokenSpreader::ImportanceChanges *) data.importance_changes->lookup(handles[1]))->wages, 0.125));
        EXPECT_TRUE(importance_equals(((TokenSpreader::ImportanceChanges *) data.importance_changes->lookup(handles[2]))->wages, 0.250));
        EXPECT_TRUE(importance_equals(((TokenSpreader::ImportanceChanges *) data.importance_changes->lookup(handles[3]))->wages, 0.125));
        EXPECT_TRUE(importance_equals(((TokenSpreader::ImportanceChanges *) data.importance_changes->lookup(handles[4]))->wages, 0.250));
        EXPECT_TRUE(data.importance_changes->lookup(handles[5]) == NULL);
        EXPECT_TRUE(data.importance_changes->lookup(handles[6]) == NULL);
        EXPECT_TRUE(data.importance_changes->lookup(handles[7]) == NULL);
        EXPECT_TRUE(data.importance_changes->lookup(handles[8]) == NULL);
        EXPECT_TRUE(data.importance_changes->lookup(handles[9]) == NULL);

        delete spreader;
    }
}

static HebbianNetwork *build_test_network(string *handles) {

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

    request = new dasproto::HandleList();
    request->set_hebbian_network((unsigned long) network);
    request->add_list(handles[1]);
    request->add_list(handles[2]);
    request->add_list(handles[4]);
    request->add_list(handles[5]);
    updater->correlation(request);

    return network;
}

TEST(TokenSpreader, spread_stimuli) {

    // --------------------------------------------------------------------
    // NOTE TO REVIEWER: I left debug messages because this code extremely
    //                   error prone and difficult to debug. Probably we'll
    //                   need to return to this test to make it pass when
    //                   we make changes in the tested code.
    // --------------------------------------------------------------------
    // Build and check network

    string *handles = build_handle_space(6, true);
    for (unsigned int i = 0; i < 6; i++) {
        cout << i << ": " << handles[i] << endl;
    }

    HebbianNetwork *network = build_test_network(handles);

    unsigned int expected[6][6] = {
        {0, 1, 1, 1, 0, 0},
        {1, 0, 2, 1, 1, 1},
        {1, 2, 0, 1, 1, 1},
        {1, 1, 1, 0, 0, 0},
        {0, 1, 1, 0, 0, 1},
        {0, 1, 1, 0, 1, 0},
    };
    for (unsigned int i = 0; i < 6; i++) {
        EXPECT_TRUE(importance_equals(network->get_node_importance(handles[i]), 0.0000));
        if (i == 1 || i == 2) {
            EXPECT_TRUE(network->get_node_count(handles[i]) == 2);
        } else {
            EXPECT_TRUE(network->get_node_count(handles[i]) == 1);
        }
        for (unsigned int j = 0; j < 6; j++) {
            cout << i << ", " << j << ": " << expected[i][j] << " " << network->get_asymmetric_edge_count(handles[i], handles[j]) << endl;
            EXPECT_TRUE(network->get_asymmetric_edge_count(handles[i], handles[j]) == expected[i][j]);
        }
    }

    // ----------------------------------------------------------
    // Build and process simulus spreading request

    dasproto::HandleCount *request;
    TokenSpreader *spreader = \
        (TokenSpreader *) StimulusSpreader::factory(StimulusSpreaderType::TOKEN);

    request = new dasproto::HandleCount();
    request->set_hebbian_network((unsigned long) network);
    (*request->mutable_map())[handles[0]] = 1;
    (*request->mutable_map())[handles[1]] = 1;
    (*request->mutable_map())[handles[2]] = 1;
    (*request->mutable_map())[handles[3]] = 1;
    (*request->mutable_map())[handles[4]] = 1;
    (*request->mutable_map())[handles[5]] = 1;
    (*request->mutable_map())["SUM"] = 6;
    unsigned int SUM = (*request->mutable_map())["SUM"];
    spreader->spread_stimuli(request);

    // ----------------------------------------------------------
    // Compute expected value for importance of each node

    unsigned int arity[6];
    arity[0] = 3;
    arity[1] = 5;
    arity[2] = 5;
    arity[3] = 3;
    arity[4] = 3;
    arity[5] = 3;
    unsigned int max_arity = 5;

    double base_importance = (double) 1 / 6;
    double rent = base_importance * AttentionBrokerServer::RENT_RATE;
    double total_rent = rent * 6;
    double total_wages = total_rent;

    cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << endl;
    cout << "XXX Expected total rent: " << total_rent << endl;
    cout << "XXX Expected rent rate: " << AttentionBrokerServer::RENT_RATE << endl;

    double wages[6];
    for (unsigned int i = 0; i < 6; i++) {
        wages[i] = ((double) ((*request->mutable_map())[handles[i]])) / SUM * total_wages;
    }

    double updated[6];
    for (unsigned int i = 0; i < 6; i++) {
        updated[i] = base_importance + wages[i] - rent;
    }

    double to_spread[6];
    for (unsigned int i = 0; i < 6; i++) {
        double arity_ratio = (double) arity[i] / max_arity;
        double lb = AttentionBrokerServer::SPREADING_RATE_LOWERBOUND;
        double ub = AttentionBrokerServer::SPREADING_RATE_UPPERBOUND;
        double spreading_rate = lb + (arity_ratio * (ub - lb));
        to_spread[i] = updated[i] * spreading_rate;
        cout << "XXX Total to spread: " << to_spread[i] << endl;
    }

    double sum_weight[6] = {3.0, 3.0, 3.0, 3.0, 3.0, 3.0};
    double weight[6][6] = {
        {0.0, 1.0, 1.0, 1.0, 0.0, 0.0},
        {0.5, 0.0, 1.0, 0.5, 0.5, 0.5},
        {0.5, 1.0, 0.0, 0.5, 0.5, 0.5},
        {1.0, 1.0, 1.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 1.0, 0.0, 0.0, 1.0},
        {0.0, 1.0, 1.0, 0.0, 1.0, 0.0},
    };
    for (unsigned int i = 0; i < 6; i++) {
        for (unsigned int j = 0; j < 6; j++) {
            if (i != j) {
                cout << "XXX weight[" << i << "][" << j << "]: " << weight[i][j] << endl;
            }
        }
        cout << "XXX sum_weight[" << i << "]: " << sum_weight[i] << endl;
    }

    double received[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    for (unsigned int i = 0; i < 6; i++) {
        for (unsigned int j = 0; j < 6; j++) {
            if (i != j) {
                double weight_ratio = weight[i][j] / sum_weight[i];
                double stimulus = weight_ratio * to_spread[i];
                cout << "XXX stimulus[" << i << "][" << j << "]: " << stimulus << endl;
                received[j] += stimulus;
            }
        }
    }

    double expected_importance[6];
    for (unsigned int i = 0; i < 6; i++) {
        expected_importance[i] = updated[i] - to_spread[i] + received[i];
    }
    cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << endl;

    // ----------------------------------------------------------
    // Compare result with expected result

    for (unsigned int i = 0; i < 6; i++) {
        cout << expected_importance[i] << " " << network->get_node_importance(handles[i]) << endl;
        EXPECT_TRUE(importance_equals(network->get_node_importance(handles[i]), expected_importance[i]));
    }
}
