#include <cstdlib>
#include <map>

#include "gtest/gtest.h"
#include "common.pb.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"

#include "Utils.h"
#include "test_utils.h"
#include "RequestQueue.h"
#include "WorkerThreads.h"
#include "HebbianNetwork.h"

using namespace attention_broker_server;

class TestRequestQueue: public RequestQueue {
    public:
        TestRequestQueue() : RequestQueue() {
        }
        unsigned int test_current_count() {
            return current_count();
        }
};


TEST(WorkerThreads, basics) {
    
    dasproto::HandleCount *handle_count;
    dasproto::HandleList *handle_list;

    unsigned int num_requests = 1000000;
    unsigned int wait_for_threads_ms = 500;

    for (double stimulus_prob: {0.0, 0.25, 0.5, 0.75, 1.0}) {
        TestRequestQueue *stimulus = new TestRequestQueue();
        TestRequestQueue *correlation = new TestRequestQueue();
        WorkerThreads *pool = new WorkerThreads(stimulus, correlation);
        for (unsigned int i = 0; i < num_requests; i++) {
            if (Utils::flip_coin(stimulus_prob)) {
                handle_count = new dasproto::HandleCount();
                stimulus->enqueue(handle_count);
            } else {
                handle_list = new dasproto::HandleList();
                handle_list->set_hebbian_network((long) NULL);
                correlation->enqueue(handle_list);
            }
        }
        this_thread::sleep_for(chrono::milliseconds(wait_for_threads_ms));
        EXPECT_TRUE(stimulus->test_current_count() == 0);
        EXPECT_TRUE(correlation->test_current_count() == 0);
        pool->graceful_stop();
        delete pool;
        delete stimulus;
        delete correlation;
    }
}

TEST(WorkerThreads, hebbian_network_updater_basics) {

    dasproto::HandleList *handle_list;
    map<string, unsigned int> node_count;
    map<string, unsigned int> edge_count;
    HebbianNetwork network;

    TestRequestQueue *stimulus = new TestRequestQueue();
    TestRequestQueue *correlation = new TestRequestQueue();
    WorkerThreads *pool = new WorkerThreads(stimulus, correlation);

    handle_list = new dasproto::HandleList();
    string h1 = random_handle();
    string h2 = random_handle();
    string h3 = random_handle();
    string h4 = random_handle();
    handle_list->add_list(h1);
    handle_list->add_list(h2);
    handle_list->add_list(h3);
    handle_list->add_list(h4);
    handle_list->set_hebbian_network((long) &network);
    correlation->enqueue(handle_list);

    handle_list = new dasproto::HandleList();
    string h5 = random_handle();
    handle_list->add_list(h1);
    handle_list->add_list(h2);
    handle_list->add_list(h5);
    handle_list->set_hebbian_network((long) &network);
    correlation->enqueue(handle_list);

    handle_list = new dasproto::HandleList();
    handle_list->add_list(h2);
    handle_list->add_list(h5);
    handle_list->set_hebbian_network((long) &network);
    correlation->enqueue(handle_list);

    string h6 = random_handle();
    handle_list = new dasproto::HandleList();
    handle_list->add_list(h6);
    handle_list->add_list(h6);
    handle_list->set_hebbian_network((long) &network);
    correlation->enqueue(handle_list);

    handle_list = new dasproto::HandleList();
    handle_list->add_list(h1);
    handle_list->add_list(h1);
    handle_list->set_hebbian_network((long) &network);
    correlation->enqueue(handle_list);

    this_thread::sleep_for(chrono::milliseconds(1000));
    EXPECT_TRUE(correlation->test_current_count() == 0);

    EXPECT_TRUE(network.get_node_count(h1) == 4);
    EXPECT_TRUE(network.get_node_count(h2) == 3);
    EXPECT_TRUE(network.get_node_count(h3) == 1);
    EXPECT_TRUE(network.get_node_count(h4) == 1);
    EXPECT_TRUE(network.get_node_count(h5) == 2);
    EXPECT_TRUE(network.get_node_count(h6) == 2);

    EXPECT_TRUE(network.get_asymmetric_edge_count(h1, h2) == 2);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h2, h1) == 2);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h1, h3) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h3, h1) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h1, h4) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h4, h1) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h1, h5) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h5, h1) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h2, h5) == 2);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h5, h2) == 2);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h2, h3) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h3, h2) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h2, h4) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h4, h2) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h3, h4) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h4, h3) == 1);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h1, h1) == 0);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h2, h2) == 0);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h5, h3) == 0);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h3, h5) == 0);
    EXPECT_TRUE(network.get_asymmetric_edge_count(h6, h6) == 0);

    pool->graceful_stop();

    delete pool;
    delete stimulus;
    delete correlation;
}

TEST(WorkerThreads, hebbian_network_updater_stress) {

    #define HANDLE_SPACE_SIZE ((unsigned int) 100)
    unsigned int num_requests = 10;
    unsigned int max_handles_per_request = 10;
    unsigned int wait_for_worker_threads_ms = 3000;

    
    dasproto::HandleList *handle_list;
    string handles[HANDLE_SPACE_SIZE];
    map<string, unsigned int> node_count;
    map<string, unsigned int> edge_count;
    HebbianNetwork *network = new HebbianNetwork();
    for (unsigned int i = 0; i < HANDLE_SPACE_SIZE; i++) {
        handles[i] = random_handle();
    }

    TestRequestQueue *stimulus = new TestRequestQueue();
    TestRequestQueue *correlation = new TestRequestQueue();
    WorkerThreads *pool = new WorkerThreads(stimulus, correlation);
    for (unsigned int i = 0; i < num_requests; i++) {
        handle_list = new dasproto::HandleList();
        unsigned int num_handles = (rand() % (max_handles_per_request - 1)) + 2;
        for (unsigned int j = 0; j < num_handles; j++) {
            string h = handles[rand() % HANDLE_SPACE_SIZE];
            handle_list->add_list(h);
            if (node_count.find(h) == node_count.end()) {
                node_count[h] = 0;
            }
            node_count[h] = node_count[h] + 1;
        }
        for (const string &h1: handle_list->list()) {
            for (const string &h2: handle_list->list()) {
                string composite;
                if (h1.compare(h2) < 0) {
                    composite = h1 + h2;
                    if (edge_count.find(composite) == edge_count.end()) {
                        edge_count[composite] = 0;
                    }
                    edge_count[composite] = edge_count[composite] + 1;
                }
            }
        }
        handle_list->set_hebbian_network((long) network);
        correlation->enqueue(handle_list);
    }

    this_thread::sleep_for(chrono::milliseconds(wait_for_worker_threads_ms));
    EXPECT_TRUE(stimulus->test_current_count() == 0);
    EXPECT_TRUE(correlation->test_current_count() == 0);
    return;

    for (unsigned int i = 0; i < HANDLE_SPACE_SIZE; i++) {
        for (unsigned int j = 0; j < HANDLE_SPACE_SIZE; j++) {
            string h1 = handles[i];
            string h2 = handles[j];
            string composite;
            if (h1.compare(h2) < 0) {
                composite = h1 + h2;
            } else {
                composite = h2 + h1;
            }
            EXPECT_TRUE(network->get_node_count(h1) == node_count[h1]);
            EXPECT_TRUE(network->get_node_count(h2) == node_count[h2]);
            EXPECT_TRUE(network->get_asymmetric_edge_count(h1, h2) == edge_count[composite]);
            EXPECT_TRUE(network->get_asymmetric_edge_count(h2, h1) == edge_count[composite]);
        }
    }
    pool->graceful_stop();
    delete network;
    delete pool;
    delete stimulus;
    delete correlation;
}
