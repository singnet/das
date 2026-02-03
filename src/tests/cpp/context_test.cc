#include "AtomDBSingleton.h"
#include "AttentionBrokerClient.h"
#include "ContextBrokerProcessor.h"
#include "ContextBrokerProxy.h"
#include "Hasher.h"
#include "PatternMatchingQueryProcessor.h"
#include "QueryAnswer.h"
#include "RedisMongoDB.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"
#include "TestConfig.h"
#include "UntypedVariable.h"
#include "Utils.h"
#include "gtest/gtest.h"
#include "test_utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace attention_broker;
using namespace context_broker;
using namespace query_engine;

class ContextTestEnvironment : public ::testing::Environment {
   public:
    void SetUp() override {
        TestConfig::load_environment();
        AtomDBSingleton::init();
    }

    void TearDown() override {}
};

class ContextTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto atomdb = AtomDBSingleton::get_instance();
        db = dynamic_pointer_cast<RedisMongoDB>(atomdb);
        ASSERT_NE(db, nullptr) << "Failed to cast AtomDB to RedisMongoDB";
    }

    void TearDown() override {}

    shared_ptr<RedisMongoDB> db;
};

TEST_F(ContextTest, ContextBroker) {
    string query_agent_id = "localhost:50000";
    string context_processor_id = "localhost:50001";
    string context_client_id = "localhost:50002";

    ServiceBusSingleton::init(context_processor_id, query_agent_id, 50003, 50999);
    shared_ptr<ServiceBus> context_bus = ServiceBusSingleton::get_instance();
    context_bus->register_processor(make_shared<ContextBrokerProcessor>());
    Utils::sleep(500);

    shared_ptr<ServiceBus> query_bus = make_shared<ServiceBus>(query_agent_id, context_processor_id);
    Utils::sleep(500);
    query_bus->register_processor(make_shared<PatternMatchingQueryProcessor>());
    Utils::sleep(500);

    shared_ptr<ServiceBus> client_bus = make_shared<ServiceBus>(context_client_id, context_processor_id);
    Utils::sleep(500);

    string node1 = Hasher::node_handle("Symbol", "\"human\"");                // 0
    string node2 = Hasher::node_handle("Symbol", "\"monkey\"");               // 1
    string node3 = Hasher::node_handle("Symbol", "\"chimp\"");                // 2
    string node4 = Hasher::node_handle("Symbol", "\"ent\"");                  // 3
    string node5 = Hasher::node_handle("Symbol", "Similarity");               // 4
    string link1 = Hasher::link_handle("Expression", {node5, node1, node2});  // 5
    string link2 = Hasher::link_handle("Expression", {node5, node1, node3});  // 6
    string link3 = Hasher::link_handle("Expression", {node5, node1, node4});  // 7

    vector<string> tokens = {"LINK_TEMPLATE",
                             "Expression",
                             "3",
                             "NODE",
                             "Symbol",
                             "Similarity",
                             "NODE",
                             "Symbol",
                             "\"human\"",
                             "VARIABLE",
                             "v1"};
    vector<string> handles = {node1, node2, node3, node4, node5, link1, link2, link3};
    vector<float> importance;

    QueryAnswerElement v1("v1");
    QueryAnswerElement toplevel_link(0);

    vector<pair<QueryAnswerElement, QueryAnswerElement>> determiner_schema = {{toplevel_link, v1}};
    vector<QueryAnswerElement> stimulus_schema = {v1};

    shared_ptr<ContextBrokerProxy> context_proxy =
        make_shared<ContextBrokerProxy>(random_handle(), tokens, determiner_schema, stimulus_schema);

    client_bus->issue_bus_command(context_proxy);
    LOG_INFO("Waiting for context creation to finish...");
    auto count = 0;
    while (!context_proxy->is_context_created()) {
        Utils::sleep();
        count++;
        if (count > 100) {
            throw std::runtime_error("Context creation timed out");
        }
    }
    LOG_INFO("Context creation finished");

    importance.clear();
    AttentionBrokerClient::get_importance(handles, context_proxy->get_key(), importance);
    for (unsigned int i = 0; i < handles.size(); i++) {
        // "human" and "Similarity" not stimulated
        if (i == 0 || i == 4) {
            EXPECT_TRUE(importance[i] == 0);
        } else {
            EXPECT_TRUE(importance[i] > 0);
        }
        cout << "importance_check_0[" << i << "]: " << importance[i] << endl;
    }

    // To be used to test AB parameters set to Zero
    vector<float> last_importances = {};

    AttentionBrokerClient::stimulate({{node2, 1}}, context_proxy->get_key());
    importance.clear();
    AttentionBrokerClient::get_importance(handles, context_proxy->get_key(), importance);
    for (unsigned int i = 0; i < handles.size(); i++) {
        // "human" and "Similarity" still not stimulated
        if (i == 0 || i == 4) {
            EXPECT_TRUE(importance[i] == 0);
        } else {
            EXPECT_TRUE(importance[i] > 0);
        }
        cout << "importance_check_1[" << i << "]: " << importance[i] << endl;
        last_importances.push_back(importance[i]);
    }
    // "monkey" must be more important than "chimp" and "ent" now
    EXPECT_TRUE(importance[1] > importance[2] && importance[1] > importance[3]);
    // "monkey" link must be more important than "chimp" and "ent" links now
    EXPECT_TRUE(importance[5] > importance[6] && importance[5] > importance[7]);

    // --- Commented out as this could impact other tests when running test-all ---

    // LOG_INFO("Setting AttentionBroker parameters to Zero");
    // context_proxy->attention_broker_set_parameters(0, 0, 0);
    // while (!context_proxy->attention_broker_set_parameters_finished()) {
    //     Utils::sleep();
    // }
    // LOG_INFO("AttentionBroker parameters set to Zero");

    // // Even stimulating "monkey" should not change the importances
    // AttentionBrokerClient::stimulate({{node2, 1}}, context_proxy->get_key());
    // importance.clear();
    // AttentionBrokerClient::get_importance(handles, context_proxy->get_key(), importance);
    // for (unsigned int i = 0; i < handles.size(); i++) {
    //     EXPECT_TRUE(importance[i] == last_importances[i]);
    //     cout << "importance_check_2[" << i << "]: " << importance[i] << endl;
    // }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new ContextTestEnvironment());
    return RUN_ALL_TESTS();
}
