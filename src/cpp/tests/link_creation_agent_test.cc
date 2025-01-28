#include "agent.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace std;
using namespace link_creation_agent;

class LinkCreationAgentTest : public ::testing::Test {
protected:
    LinkCreationAgent* agent;

    void SetUp() override {
        // Create a temporary config file for testing
        ofstream config_file("test_config.cfg");
        config_file << "default_interval=1\n";
        config_file << "thread_count=1\n";
        config_file << "query_node_client_id=test_client_id\n";
        config_file << "query_node_server_id=test_server_id\n";
        config_file << "link_creation_server_id=test_link_creation_server_id\n";
        config_file << "das_client_id=test_das_client_id\n";
        config_file << "requests_buffer_file=test_buffer.bin\n";
        config_file << "context=test_context\n";
        config_file.close();

        agent = new LinkCreationAgent("test_config.cfg");
    }

    void TearDown() override {
        delete agent;
        remove("test_config.cfg");
        remove("test_buffer.bin");
    }
};



TEST_F(LinkCreationAgentTest, TestRun) {
    // Simulate a request
    vector<string> request = {"query1", "LINK_CREATE", "query2", "10", "5", "test_context", "true"};
    LinkCreationAgentRequest* lca_request = LinkCreationAgent::create_request(request);
    EXPECT_EQ(lca_request->query, vector<string>({"query1"}));
    EXPECT_EQ(lca_request->link_template, vector<string>({"LINK_CREATE", "query2"}));
    EXPECT_EQ(lca_request->max_results, 10);
    EXPECT_EQ(lca_request->repeat, 5);
    EXPECT_EQ(lca_request->context, "test_context");
    EXPECT_EQ(lca_request->update_attention_broker, true);


}

