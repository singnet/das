#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>

#include "AtomDB.h"
#include "AtomDBSingleton.h"
#include "BusCommandProcessor.h"
#include "BusCommandProxy.h"
#include "FitnessFunctionRegistry.h"
#include "InferenceProcessor.h"
#include "LinkCreationRequestProcessor.h"
#include "MockAtomDB.h"
#include "MockServiceBus.h"
#include "QueryEvolutionProcessor.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

using namespace std;
using namespace inference_agent;
using namespace commons;
using namespace distributed_algorithm_node;
using namespace atomdb;
using namespace evolution;
using namespace link_creation_agent;
using namespace atoms;

class MockInferenceProxy : public InferenceProxy {
   public:
    MockInferenceProxy(const vector<string>& tokens) : InferenceProxy(tokens) {}
    MOCK_METHOD(unsigned int, get_serial, (), (override));
    MOCK_METHOD(string, peer_id, (), (override));
};

class InferenceAgentTest : public ::testing::Test {
   protected:
    static void SetUpTestSuite() {
        AtomDBSingleton::provide(nullptr);
        FitnessFunctionRegistry::initialize_statics();
    }

    void SetUp() override {
        ServiceBusSingleton::provide(
            move(make_shared<MockServiceBus>("localhost:40038", "localhost:40039")));
        AtomDBSingleton::provide(move(make_shared<AtomDBMock>()));
    }

    void TearDown() override {
        ServiceBusSingleton::provide(nullptr);
        AtomDBSingleton::provide(nullptr);
    }
};

TEST_F(InferenceAgentTest, TestProofOfImplication) {
    GTEST_SKIP()
        << "Skipping test, due to modifications on InferenceAgent logic, it needs to be rewritten";
    auto inference_agent = new InferenceAgent();
    vector<string> tokens = {"PROOF_OF_IMPLICATION", "handle1", "handle2", "1", "context"};
    auto inference_proxy = make_shared<MockInferenceProxy>(tokens);

    vector<string> calls_list = {"link_creation", "query_evolution"};
    int count = 0;
    int expected_calls = calls_list.size();
    auto mock_service_bus = dynamic_cast<MockServiceBus*>(ServiceBusSingleton::get_instance().get());
    EXPECT_CALL(*mock_service_bus, issue_bus_command(testing::_))
        .Times(expected_calls)
        .WillRepeatedly(
            ::testing::Invoke([&calls_list, &count](shared_ptr<BusCommandProxy> command_proxy) {
                EXPECT_EQ(command_proxy->get_command(), calls_list[count]);
                count++;
            }));

    inference_agent->process_inference_request(inference_proxy);
    while (count < expected_calls) {
        Utils::sleep(100);
    }
    inference_agent->stop();
    delete inference_agent;
}

TEST_F(InferenceAgentTest, TestProofOfEquivalence) {
    GTEST_SKIP()
        << "Skipping test, due to modifications on InferenceAgent logic, it needs to be rewritten";
    auto inference_agent = new InferenceAgent();
    vector<string> tokens = {"PROOF_OF_EQUIVALENCE", "handle1", "handle2", "1", "context"};
    auto inference_proxy = make_shared<MockInferenceProxy>(tokens);

    vector<string> calls_list = {"link_creation", "query_evolution"};
    int count = 0;
    int expected_calls = calls_list.size();
    auto mock_service_bus = dynamic_cast<MockServiceBus*>(ServiceBusSingleton::get_instance().get());
    EXPECT_CALL(*mock_service_bus, issue_bus_command(testing::_))
        .Times(expected_calls)
        .WillRepeatedly(
            ::testing::Invoke([&calls_list, &count](shared_ptr<BusCommandProxy> command_proxy) {
                EXPECT_EQ(command_proxy->get_command(), calls_list[count]);
                count++;
            }));

    inference_agent->process_inference_request(inference_proxy);
    while (count < expected_calls) {
        Utils::sleep(100);
    }
    inference_agent->stop();
    delete inference_agent;
}

TEST_F(InferenceAgentTest, TestInferenceRequests) {
    ProofOfImplication proof_of_implication("handle1", "handle2", 1, "context");
    auto requests = proof_of_implication.get_requests();
    auto dic_request = proof_of_implication.get_distributed_inference_control_request();
    EXPECT_EQ(requests.size(), 1);
    EXPECT_EQ(Utils::join(dic_request, ' '),
              "AND 2 LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION ATOM handle1 VARIABLE V0 "
              "LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION VARIABLE V0 ATOM handle2");

    ProofOfImplication proof_of_implication2("handle1", "handle2", 2, "context");
    requests = proof_of_implication2.get_requests();
    dic_request = proof_of_implication2.get_distributed_inference_control_request();
    EXPECT_EQ(requests.size(), 1);
    // clang-format off
    EXPECT_EQ(Utils::join(dic_request, ' '),
              "OR 2 "
                "AND 3 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol IMPLICATION "
                        "ATOM handle1 "
                        "VARIABLE V0 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol IMPLICATION "
                        "VARIABLE V0 "
                        "VARIABLE V1 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol IMPLICATION "
                        "VARIABLE V1 "
                        "ATOM handle2 "
                "AND 2 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol IMPLICATION "
                        "ATOM handle1 "
                        "VARIABLE V0 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol IMPLICATION "
                        "VARIABLE V0 "
                        "ATOM handle2");
    // clang-format on
    ProofOfEquivalence proof_of_equivalence("handle1", "handle2", 1, "context");
    requests = proof_of_equivalence.get_requests();
    dic_request = proof_of_equivalence.get_distributed_inference_control_request();
    EXPECT_EQ(requests.size(), 1);
    EXPECT_EQ(Utils::join(dic_request, ' '),
              "AND 2 LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE ATOM handle1 VARIABLE V0 "
              "LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE VARIABLE V0 ATOM handle2");

    ProofOfEquivalence poe2("handle1", "handle2", 3, "context");
    requests = poe2.get_requests();
    dic_request = poe2.get_distributed_inference_control_request();
    EXPECT_EQ(requests.size(), 1);
    // clang-format off
    EXPECT_EQ(Utils::join(dic_request, ' '),
              "OR 3 "
                "AND 4 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol EQUIVALENCE "
                        "ATOM handle1 "
                        "VARIABLE V0 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol EQUIVALENCE "
                        "VARIABLE V0 "
                        "VARIABLE V1 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol EQUIVALENCE "
                        "VARIABLE V1 "
                        "VARIABLE V2 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol EQUIVALENCE "
                        "VARIABLE V2 "
                        "ATOM handle2 "
                "AND 3 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol EQUIVALENCE "
                        "ATOM handle1 "
                        "VARIABLE V0 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol EQUIVALENCE "
                        "VARIABLE V0 "
                        "VARIABLE V1 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol EQUIVALENCE "
                        "VARIABLE V1 "
                        "ATOM handle2 "
                "AND 2 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol EQUIVALENCE "
                        "ATOM handle1 "
                        "VARIABLE V0 "
                    "LINK_TEMPLATE Expression 3 "
                        "NODE Symbol EQUIVALENCE "
                        "VARIABLE V0 "
                        "ATOM handle2");
    // clang-format on
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
