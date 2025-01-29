#include "agent.h"
#include <gtest/gtest.h>
#include <thread>
#include <fstream>
#include <chrono>

using namespace std;
using namespace link_creation_agent;

class LinkCreationAgentTest : public ::testing::Test {
protected:
    LinkCreationAgent* agent;

    void SetUp() override {
       // Create a temporary config file for testing
        ofstream config_file("test_config.cfg");
        config_file << "requests_interval_seconds=1\n";
        config_file << "link_creation_agent_thread_count=1\n";
        config_file << "query_agent_client_id=localhost:8080\n";
        config_file << "query_agent_server_id=localhost:8081\n";
        config_file << "link_creation_agent_server_id=localhost:8082\n";
        config_file << "das_agent_client_id=localhost:8083\n";
        config_file << "das_agent_server_id=localhost:8083\n";
        config_file << "requests_buffer_file=test_buffer.bin\n";
        config_file.close();
    }

    void TearDown() override {
        remove("test_config.cfg");
        remove("test_buffer.bin");
    }
};



TEST_F(LinkCreationAgentTest, TestRequest) {
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


TEST_F(LinkCreationAgentTest, TestConfig){
        agent = new LinkCreationAgent("test_config.cfg");
        delete agent;
}


TEST_F(LinkCreationAgentTest, TestOutputBuffer){
        agent = new LinkCreationAgent("test_config.cfg");
        delete agent;
}


