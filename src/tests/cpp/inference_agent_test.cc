#include "inference_agent.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>

#include "MessageBroker.h"
#include "Utils.h"

using namespace std;
using namespace inference_agent;
using namespace commons;
using namespace distributed_algorithm_node;

class MockInferenceAgentNode : public InferenceAgentNode {
   public:
    MockInferenceAgentNode(const string& node_id) : InferenceAgentNode(node_id) {}
    MockInferenceAgentNode(const string& node_id, const string& server_id)
        : InferenceAgentNode(node_id, server_id) {}
    MockInferenceAgentNode(const string& node_id, MessageBrokerType messaging_backend)
        : InferenceAgentNode(node_id, messaging_backend) {}

    // MOCK_METHOD(void, send_message, (std::vector<std::string> args), (override));
    // MOCK_METHOD(bool, is_answers_empty, (), (override));
};

class MockLinkCreationAgentNode : public LinkCreationAgentNode {
   public:
    MockLinkCreationAgentNode(const string& node_id, const string& server_id)
        : LinkCreationAgentNode(node_id, server_id) {}

    MOCK_METHOD(void, send_message, (std::vector<std::string> args), (override));
    MOCK_METHOD(std::vector<std::string>, pop_request, (), (override));
};

class MockDasAgentNode : public DasAgentNode {
   public:
    MockDasAgentNode(const string& node_id, const string& server_id)
        : DasAgentNode(node_id, server_id) {}

    MOCK_METHOD(void, create_link, (std::vector<std::string> & request), (override));
};

class MockDistributedInferenceControlAgentNode : public DistributedInferenceControlAgentNode {
   public:
    MockDistributedInferenceControlAgentNode(const string& node_id, const string& server_id)
        : DistributedInferenceControlAgentNode(node_id, server_id) {}

    MOCK_METHOD(void,
                send_inference_control_request,
                (std::vector<std::string> inference_control_request, std::string response_node_id),
                (override));
};

class InferenceAgentTest : public ::testing::Test {
   protected:
    InferenceAgent* agent;
    MockInferenceAgentNode* inference_node_server;
    MockInferenceAgentNode* inference_node_client;
    MockLinkCreationAgentNode* link_creation_node_client;
    MockDasAgentNode* das_node_client;
    MockDistributedInferenceControlAgentNode* distributed_inference_control_node_client;

    void SetUp() override {
        // Create a temporary config file for testing
        ofstream config_file("inference_test_config.cfg");
        config_file << "inference_node_id=localhost:4000\n";
        config_file << "das_client_id=localhost:9090\n";
        config_file << "das_server_id=localhost:9091\n";
        config_file << "distributed_inference_control_node_id=localhost:5000\n";
        config_file << "distributed_inference_control_node_server_id=localhost:5001\n";
        config_file << "link_creation_agent_server_id=localhost:9080\n";
        config_file << "link_creation_agent_client_id=localhost:9081\n";
        config_file.close();

        string inference_node_client_id = Utils::get_environment("INFERENCE_CLIENT");
        inference_node_client_id =
            inference_node_client_id.empty() ? "localhost:1110" : inference_node_client_id;
        string inference_node_server_id = Utils::get_environment("INFERENCE_SERVER");
        inference_node_server_id =
            inference_node_server_id.empty() ? "localhost:1120" : inference_node_server_id;
        string link_creation_node_client_id = Utils::get_environment("LINK_CREATION_CLIENT");
        link_creation_node_client_id =
            link_creation_node_client_id.empty() ? "localhost:1130" : link_creation_node_client_id;
        string link_creation_node_server_id = Utils::get_environment("LINK_CREATION_SERVER");
        link_creation_node_server_id =
            link_creation_node_server_id.empty() ? "localhost:1140" : link_creation_node_server_id;
        string das_node_client_id = Utils::get_environment("DAS_CLIENT");
        das_node_client_id = das_node_client_id.empty() ? "localhost:1150" : das_node_client_id;
        string das_node_server_id = Utils::get_environment("DAS_SERVER");
        das_node_server_id = das_node_server_id.empty() ? "localhost:1160" : das_node_server_id;
        string dic_node_client_id = Utils::get_environment("DIC_CLIENT");
        dic_node_client_id = dic_node_client_id.empty() ? "localhost:1170" : dic_node_client_id;
        string dic_node_server_id = Utils::get_environment("DIC_SERVER");
        dic_node_server_id = dic_node_server_id.empty() ? "localhost:1180" : dic_node_server_id;

        inference_node_server = new MockInferenceAgentNode(inference_node_server_id);
        inference_node_client =
            new MockInferenceAgentNode(inference_node_client_id, inference_node_server_id);

        link_creation_node_client =
            new MockLinkCreationAgentNode(link_creation_node_client_id, link_creation_node_server_id);
        das_node_client = new MockDasAgentNode(das_node_client_id, das_node_server_id);
        distributed_inference_control_node_client =
            new MockDistributedInferenceControlAgentNode(dic_node_client_id, dic_node_server_id);
    }

    void TearDown() override { remove("inference_test_config.cfg"); }

    void clear_mocks() {
        delete inference_node_server;
        delete inference_node_client;
        delete link_creation_node_client;
        delete das_node_client;
        delete distributed_inference_control_node_client;
    }
};

TEST_F(InferenceAgentTest, TestConfig) {
    agent = new InferenceAgent("inference_test_config.cfg");
    delete agent;
    clear_mocks();
}

TEST_F(InferenceAgentTest, TestProofOfImplicationOrEquivalence) {
    EXPECT_CALL(*link_creation_node_client, send_message(testing::_))
        .Times(2)
        .WillRepeatedly(::testing::Invoke(
            [](const vector<string>& message) { EXPECT_EQ(message[0], "LINK_TEMPLATE"); }));

    EXPECT_CALL(*distributed_inference_control_node_client,
                send_inference_control_request(testing::_, testing::_))
        .WillOnce(::testing::Invoke([](const vector<string>& message, std::string response_node_id) {
            EXPECT_EQ(message[0], "OR");
        }));

    InferenceAgent agent(
        dynamic_cast<InferenceAgentNode*>(inference_node_server),
        dynamic_cast<LinkCreationAgentNode*>(link_creation_node_client),
        dynamic_cast<DasAgentNode*>(das_node_client),
        dynamic_cast<DistributedInferenceControlAgentNode*>(distributed_inference_control_node_client));

    inference_node_client->send_message(
        {"PROOF_OF_IMPLICATION_OR_EQUIVALENCE", "handle1", "handle2", "1", "context"});
    this_thread::sleep_for(chrono::milliseconds(200));  // Give some time for the agent
    InferenceAgentNode dic_client = InferenceAgentNode("localhost:1111", "localhost:1121");
    dic_client.send_message({"DISTRIBUTED_INFERENCE_FINISHED"});
    this_thread::sleep_for(chrono::milliseconds(200));  // Give some time for the agent
    agent.stop();
}

TEST_F(InferenceAgentTest, TestProofOfImplication) {
    EXPECT_CALL(*link_creation_node_client, send_message(testing::_))
        .Times(2)
        .WillOnce(
            ::testing::Invoke([](const vector<string>& message) { EXPECT_EQ(message[0], "AND"); }));

    EXPECT_CALL(*distributed_inference_control_node_client,
                send_inference_control_request(testing::_, testing::_))
        .WillOnce(::testing::Invoke([](const vector<string>& message, std::string response_node_id) {
            EXPECT_EQ(message[0], "OR");
        }));

    InferenceAgent agent(
        dynamic_cast<InferenceAgentNode*>(inference_node_server),
        dynamic_cast<LinkCreationAgentNode*>(link_creation_node_client),
        dynamic_cast<DasAgentNode*>(das_node_client),
        dynamic_cast<DistributedInferenceControlAgentNode*>(distributed_inference_control_node_client));

    inference_node_client->send_message({"PROOF_OF_IMPLICATION", "handle1", "handle2", "1", "context"});
    this_thread::sleep_for(chrono::milliseconds(200));  // Give some time for the agent to start
    InferenceAgentNode dic_client = InferenceAgentNode("localhost:1111", "localhost:1121");
    dic_client.send_message({"DISTRIBUTED_INFERENCE_FINISHED"});
    this_thread::sleep_for(chrono::milliseconds(200));  // Give some time for the agent
    agent.stop();
}

TEST_F(InferenceAgentTest, TestProofOfEquivalence) {
    EXPECT_CALL(*link_creation_node_client, send_message(testing::_))
        .Times(2)
        .WillOnce(
            ::testing::Invoke([](const vector<string>& message) { EXPECT_EQ(message[0], "AND"); }));

    EXPECT_CALL(*distributed_inference_control_node_client,
                send_inference_control_request(testing::_, testing::_))
        .WillOnce(::testing::Invoke([](const vector<string>& message, std::string response_node_id) {
            EXPECT_EQ(message[0], "OR");
        }));

    InferenceAgent agent(
        dynamic_cast<InferenceAgentNode*>(inference_node_server),
        dynamic_cast<LinkCreationAgentNode*>(link_creation_node_client),
        dynamic_cast<DasAgentNode*>(das_node_client),
        dynamic_cast<DistributedInferenceControlAgentNode*>(distributed_inference_control_node_client));

    inference_node_client->send_message({"PROOF_OF_EQUIVALENCE", "handle1", "handle2", "1", "context"});
    this_thread::sleep_for(chrono::milliseconds(200));  // Give some time for the agent to start
    InferenceAgentNode dic_client = InferenceAgentNode("localhost:1111", "localhost:1121");
    dic_client.send_message({"DISTRIBUTED_INFERENCE_FINISHED"});
    this_thread::sleep_for(chrono::milliseconds(200));  // Give some time for the agent
    agent.stop();
}

TEST(InferenceRequest, TestInferenceRequests) {
    ProofOfImplicationOrEquivalence proof_of_implication_or_equivalence(
        "handle1", "handle2", 1, "context");
    auto requests = proof_of_implication_or_equivalence.get_requests();
    auto dic_request = proof_of_implication_or_equivalence.get_distributed_inference_control_request();
    EXPECT_EQ(requests.size(), 1);
    EXPECT_EQ(Utils::join(requests[0], ' '),
              "LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION LINK_TEMPLATE Expression 2 NODE Symbol "
              "PREDICATE VARIABLE P LINK_TEMPLATE Expression 2 NODE Symbol CONCEPT VARIABLE C LIST 2 "
              "LINK_CREATE Expression 2 1 NODE Symbol SATISFYING_SET VARIABLE P CUSTOM_FIELD "
              "truth_value 2 strength 1.0 confidence 1.0 LINK_CREATE Expression 2 1 NODE Symbol "
              "PATTERNS VARIABLE C CUSTOM_FIELD truth_value 2 strength 1.0 confidence 1.0");
    EXPECT_EQ(Utils::join(dic_request, ' '),
              "OR 2 LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION HANDLE handle1 HANDLE handle2 "
              "LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE HANDLE handle1 HANDLE handle2");

    ProofOfImplication proof_of_implication("handle3", "handle4", 2, "context");
    requests = proof_of_implication.get_requests();
    dic_request = proof_of_implication.get_distributed_inference_control_request();
    EXPECT_EQ(requests.size(), 1);
    EXPECT_EQ(Utils::join(requests[0], ' '),
              "AND 3 LINK_TEMPLATE Expression 2 NODE Symbol SATISFYING_SET VARIABLE P1 LINK_TEMPLATE "
              "Expression 2 NODE Symbol SATISFYING_SET VARIABLE P2 NOT OR LINK_TEMPLATE Expression 3 "
              "NODE Symbol IMPLICATION LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION VARIABLE P1 "
              "VARIABLE C LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION VARIABLE P2 VARIABLE C "
              "LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION LINK_TEMPLATE Expression 3 NODE "
              "Symbol EVALUATION VARIABLE P2 VARIABLE C LINK_TEMPLATE Expression 3 NODE Symbol "
              "EVALUATION VARIABLE P1 VARIABLE C IMPLICATION_DEDUCTION");
    EXPECT_EQ(Utils::join(dic_request, ' '),
              "OR 6 AND 2 LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION HANDLE handle3 VARIABLE V1 "
              "LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION VARIABLE V1 HANDLE handle4 AND 2 "
              "LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE HANDLE handle3 VARIABLE V1 "
              "LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION VARIABLE V1 HANDLE handle4 AND 2 "
              "LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION HANDLE handle3 VARIABLE V1 "
              "LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE VARIABLE V1 HANDLE handle4 AND 2 "
              "LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE HANDLE handle3 VARIABLE V1 "
              "LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE VARIABLE V1 HANDLE handle4 "
              "LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION HANDLE handle3 HANDLE handle4 "
              "LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE HANDLE handle3 HANDLE handle4");

    ProofOfEquivalence proof_of_equivalence("handle5", "handle6", 1, "context");
    requests = proof_of_equivalence.get_requests();
    dic_request = proof_of_equivalence.get_distributed_inference_control_request();
    EXPECT_EQ(requests.size(), 1);
    EXPECT_EQ(Utils::join(requests[0], ' '),
              "AND 3 LINK_TEMPLATE Expression 2 NODE Symbol PATTERNS VARIABLE C1 LINK_TEMPLATE "
              "Expression 2 NODE Symbol PATTERNS VARIABLE C2 NOT OR LINK_TEMPLATE Expression 3 NODE "
              "Symbol EQUIVALENCE LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION VARIABLE P VARIABLE "
              "C1 LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION VARIABLE P VARIABLE C2 "
              "LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE LINK_TEMPLATE Expression 3 NODE "
              "Symbol EVALUATION VARIABLE P VARIABLE C2 LINK_TEMPLATE Expression 3 NODE Symbol "
              "EVALUATION VARIABLE P VARIABLE C1 EQUIVALENCE_DEDUCTION");
    EXPECT_EQ(Utils::join(dic_request, ' '),
              "OR 2 LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION HANDLE handle5 HANDLE handle6 "
              "LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE HANDLE handle5 HANDLE handle6");
}

TEST(InferenceRequestValidator, InvalidRequests) {
    InferenceRequestValidator validator;
    vector<string> invalid_request = {"INVALID_REQUEST"};
    EXPECT_FALSE(validator.validate(invalid_request));
    vector<string> invalid_request2 = {"PROOF_OF_IMPLICATION_OR_EQUIVALENCE", "handle1", "handle2"};
    EXPECT_FALSE(validator.validate(invalid_request2));
    vector<string> invalid_request3 = {"PROOF_OF_IMPLICATION", "handle1", "handle2"};
    EXPECT_FALSE(validator.validate(invalid_request3));
    vector<string> invalid_request4 = {"PROOF_OF_EQUIVALENCE", "handle1", "handle2"};
    EXPECT_FALSE(validator.validate(invalid_request4));
    vector<string> invalid_request5 = {"PROOF_OF_EQUIVALENCE", "handle1>", "handle2", "1"};
    EXPECT_FALSE(validator.validate(invalid_request5));
}