#include "agent.h"
#include "link_create_template.h"
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
        // ofstream config_file("test_config.cfg");
        // config_file << "requests_interval_seconds=1\n";
        // config_file << "link_creation_agent_thread_count=1\n";
        // config_file << "query_agent_client_id=localhost:8080\n";
        // config_file << "query_agent_server_id=localhost:8081\n";
        // config_file << "link_creation_agent_server_id=localhost:8082\n";
        // config_file << "das_agent_client_id=localhost:8083\n";
        // config_file << "das_agent_server_id=localhost:8083\n";
        // config_file << "requests_buffer_file=test_buffer.bin\n";
        // config_file.close();
    }

    void TearDown() override {
        // remove("test_config.cfg");
        // remove("test_buffer.bin");
    }
};



// TEST_F(LinkCreationAgentTest, TestRequest) {
//     // Simulate a request
//     vector<string> request = {"query1", "LINK_CREATE", "query2", "10", "5", "test_context", "true"};
//     LinkCreationAgentRequest* lca_request = LinkCreationAgent::create_request(request);
//     EXPECT_EQ(lca_request->query, vector<string>({"query1"}));
//     EXPECT_EQ(lca_request->link_template, vector<string>({"LINK_CREATE", "query2"}));
//     EXPECT_EQ(lca_request->max_results, 10);
//     EXPECT_EQ(lca_request->repeat, 5);
//     EXPECT_EQ(lca_request->context, "test_context");
//     EXPECT_EQ(lca_request->update_attention_broker, true);
// }


// TEST_F(LinkCreationAgentTest, TestConfig){
//         agent = new LinkCreationAgent("test_config.cfg");
//         delete agent;
// }


// TEST_F(LinkCreationAgentTest, TestOutputBuffer){
//         agent = new LinkCreationAgent("test_config.cfg");
//         delete agent;
// }


// TEST(LinkCreateTemplate, TestCustomField){
//     vector<string> args = {"CUSTOM_FIELD", "field1", "2", "value1", "value2"};
//     CustomField custom_field(args);
//     EXPECT_EQ(custom_field.get_name(), "field1");
//     vector<tuple<string, CustomFieldTypes>> values = custom_field.get_values();
//     EXPECT_EQ(values.size(), 2);
//     EXPECT_EQ(get<string>(get<1>(values[0])), "value1");
//     EXPECT_EQ(get<string>(get<1>(values[1])), "value2");
// }


TEST(LinkCreateTemplate, TestLinkCreateTemplate){
    // vector<string> link_template = {"LINK_CREATE", "link_type", "2", "1", "NODE", "type1", "value1", "VARIABLE", "var1", "CUSTOM_FIELD", "field1", "1", "value1", "value2"};
    vector<string> link_template = {"LINK_CREATE", "Similarity", "3", "2", "VARIABLE", "V1", "LINK_CREATE", "Test", "3","0", "NODE", "Symbol", "A", "VARIABLE", "V2", "LINK_CREATE", "Test2", "1","1", "NODE", "Symbol", "C", "CUSTOM_FIELD", "inter", "1", "inter_name", "inter_value", "NODE", "Symbol", "B", "CUSTOM_FIELD", "truth_value", "2", "CUSTOM_FIELD", "mean", "2", "count", "10", "avg", "0.9", "confidence", "0.9"};
    string link_template_str = "LINK_CREATE Similarity 3 1 VARIABLE V1 LINK_CREATE Test 3 0 NODE Symbol A VARIABLE V2 LINK_CREATE Test2 1 1 NODE Symbol C CUSTOM_FIELD inter 1 inter_name inter_value NODE Symbol B CUSTOM_FIELD truth_value 2 CUSTOM_FIELD mean 2 count 10 avg 0.9 confidence 0.9";
    LinkCreateTemplate lct(link_template);
    EXPECT_EQ(lct.get_link_type(), "Similarity");
    EXPECT_EQ(lct.to_string(), link_template_str);
    link_template.clear();
    link_template_str.clear();
    link_template = {"LINK_CREATE", "I", "3", "0", "VARIABLE", "V1", "LINK_CREATE", "Test", "3","0", "NODE", "Symbol", "A", "VARIABLE", "V2", "LINK_CREATE", "Test2", "1","0", "NODE", "Symbol", "C", "NODE", "Symbol", "B"};
    link_template_str = "LINK_CREATE I 3 0 VARIABLE V1 LINK_CREATE Test 3 0 NODE Symbol A VARIABLE V2 LINK_CREATE Test2 1 0 NODE Symbol C NODE Symbol B";
    LinkCreateTemplate lct2(link_template);
    EXPECT_EQ(lct2.get_link_type(), "I");
    EXPECT_EQ(lct2.to_string(), link_template_str);
    EXPECT_EQ(lct2.get_targets().size(), 3);
    EXPECT_EQ(get<Variable>(lct.get_targets()[0]).name, "V1");

}