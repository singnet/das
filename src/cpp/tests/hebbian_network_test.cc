#include <cmath>
#include <cstdlib>
#include <set>
#include <list>
#include <forward_list>

#include "gtest/gtest.h"
#include "Utils.h"
#include "test_utils.h"
#include "HebbianNetwork.h"
#include "expression_hasher.h"

using namespace attention_broker_server;
using namespace commons;

TEST(HebbianNetwork, basics) {
    
    HebbianNetwork network;

    string h1 = prefixed_random_handle("a");
    string h2 = prefixed_random_handle("b");
    string h3 = prefixed_random_handle("d");
    string h4 = prefixed_random_handle("d");
    string h5 = prefixed_random_handle("e");

    HebbianNetwork::Node *n1 = network.add_node(h1);
    HebbianNetwork::Node *n2 = network.add_node(h2);
    HebbianNetwork::Node *n3 = network.add_node(h3);
    HebbianNetwork::Node *n4 = network.add_node(h4);

    EXPECT_TRUE(network.get_node_count(h1) == 1);
    EXPECT_TRUE(network.get_node_count(h2) == 1);
    EXPECT_TRUE(network.get_node_count(h3) == 1);
    EXPECT_TRUE(network.get_node_count(h4) == 1);
    EXPECT_TRUE(network.get_node_count(h5) == 0);
    network.add_node(h5);
    EXPECT_TRUE(network.get_node_count(h5) == 1);

    network.add_symmetric_edge(h1, h2, n1, n2);
    network.add_symmetric_edge(h1, h3, n1, n3);
    network.add_symmetric_edge(h1, h4, n1, n4);
    network.add_symmetric_edge(h1, h2, n1, n2);

    EXPECT_TRUE(network.get_asymmetric_edge_count(h1, h2) == 2);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h2, h1) == 2);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h1, h3) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h3, h1) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h1, h4) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h4, h1) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h1, h5) == 0);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h5, h1) == 0);
}

TEST(HebbianNetwork, stress) {

    HebbianNetwork network;
    StopWatch timer_insertion;
    StopWatch timer_lookup;
    StopWatch timer_total;
    unsigned int handle_space_size = 500;
    unsigned int num_insertions = (handle_space_size * 2) * (handle_space_size * 2);
    unsigned int num_lookups = 10 * num_insertions;

    string *handles = build_handle_space(handle_space_size);

    timer_insertion.start();
    timer_total.start();

    for (unsigned int i = 0; i < num_insertions; i++) {
        string h1 = handles[rand() % handle_space_size];
        string h2 = handles[rand() % handle_space_size];
        HebbianNetwork::Node *n1 = network.add_node(h1);
        HebbianNetwork::Node *n2 = network.add_node(h2);
        network.add_symmetric_edge(h1, h2, n1, n2);
    }

    timer_insertion.stop();
    timer_lookup.start();

    for (unsigned int i = 0; i < num_lookups; i++) {
        string h1 = handles[rand() % handle_space_size];
        string h2 = handles[rand() % handle_space_size];
        network.get_node_count(h1);
        network.get_node_count(h2);
        network.get_asymmetric_edge_count(h1, h2);
    }

    timer_lookup.stop();
    timer_total.stop();

    cout << "==================================================================" << endl;
    cout << "Insertions: " << timer_insertion.str_time() << endl;
    cout << "Lookups: " << timer_lookup.str_time() << endl;
    cout << "Total: " << timer_total.str_time() << endl;
    cout << "==================================================================" << endl;
    //EXPECT_TRUE(false);
}

TEST(HebbianNetwork, alienate_tokens) {
    HebbianNetwork network;
    EXPECT_TRUE(network.alienate_tokens() == 1.0);
    EXPECT_TRUE(network.alienate_tokens() == 0.0);
    EXPECT_TRUE(network.alienate_tokens() == 0.0);
}

bool visit1(HandleTrie::TrieNode *node, void *data) {
    ((HebbianNetwork::Node *) node->value)->importance = 1.0;
    return false;
}

bool visit2(
    HandleTrie::TrieNode *node,
    HebbianNetwork::Node *source, 
    forward_list<HebbianNetwork::Node *> &targets, 
    unsigned int targets_size,
    ImportanceType sum_weights,
    void *data) {

    unsigned int fan_max = *((unsigned int *) data);
    double stimulus = 1.0 / (double) fan_max;
    for (auto target: targets) {
        target->importance += stimulus;
        source->importance -= stimulus;
    }
    return false;
}
