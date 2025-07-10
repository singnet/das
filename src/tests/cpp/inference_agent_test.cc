#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>

#include "AtomDBSingleton.h"
#include "AtomDB.h"
#include "BusCommandProcessor.h"
#include "BusCommandProxy.h"
#include "FitnessFunctionRegistry.h"
#include "InferenceProcessor.h"
#include "LinkCreationRequestProcessor.h"
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
// Mock service bus

class MockServiceBus : public ServiceBus {
   public:
    MockServiceBus(const string& server_id, const string& peer_address)
        : ServiceBus(server_id, peer_address) {}

    MOCK_METHOD(void, register_processor, (shared_ptr<BusCommandProcessor> processor), (override));
    MOCK_METHOD(void, issue_bus_command, (shared_ptr<BusCommandProxy> command_proxy), (override));
};

class MockInferenceProxy : public InferenceProxy {
   public:
    MockInferenceProxy(const vector<string>& tokens) : InferenceProxy(tokens) {}
    MOCK_METHOD(unsigned int, get_serial, (), (override));
};

class MockAtomDocument : public atomdb_api_types::AtomDocument {
   public:
    MOCK_METHOD(const char*, get, (const string& key), (override));
    MOCK_METHOD(const char*, get, (const string& array_key, unsigned int index), (override));
    MOCK_METHOD(unsigned int, get_size, (const string& array_key), (override));
    MOCK_METHOD(bool, contains, (const string& key), (override));
    MockAtomDocument() {
        ON_CALL(*this, get(testing::_)).WillByDefault(::testing::Return("mocked_value"));
        testing::Mock::AllowLeak(this);
        ON_CALL(*this, contains).WillByDefault(::testing::Return(true));
        ON_CALL(*this, get_size).WillByDefault(::testing::Return(1));
    }
};

class AtomDBMock : public AtomDB {
   public:
    MOCK_METHOD(shared_ptr<Atom>, get_atom, (const string& handle), (override));
    MOCK_METHOD(shared_ptr<atomdb_api_types::HandleSet>,
                query_for_pattern,
                (const LinkTemplateInterface& link_template),
                (override));
    MOCK_METHOD(shared_ptr<atomdb_api_types::HandleList>,
                query_for_targets,
                (const string& handle),
                (override));

    MOCK_METHOD(shared_ptr<atomdb_api_types::AtomDocument>,
                get_atom_document,
                (const string& handle),
                (override));
    MOCK_METHOD(shared_ptr<atomdb_api_types::AtomDocument>,
                get_node_document,
                (const string& handle),
                (override));
    MOCK_METHOD(shared_ptr<atomdb_api_types::AtomDocument>,
                get_link_document,
                (const string& handle),
                (override));

    MOCK_METHOD(vector<shared_ptr<atomdb_api_types::AtomDocument>>,
                get_atom_documents,
                (const vector<string>& handles, const vector<string>& fields),
                (override));
    MOCK_METHOD(vector<shared_ptr<atomdb_api_types::AtomDocument>>,
                get_node_documents,
                (const vector<string>& handles, const vector<string>& fields),
                (override));
    MOCK_METHOD(vector<shared_ptr<atomdb_api_types::AtomDocument>>,
                get_link_documents,
                (const vector<string>& handles, const vector<string>& fields),
                (override));

    MOCK_METHOD(bool, atom_exists, (const string& handle), (override));
    MOCK_METHOD(bool, node_exists, (const string& handle), (override));
    MOCK_METHOD(bool, link_exists, (const string& handle), (override));

    MOCK_METHOD(set<string>, atoms_exist, (const vector<string>& handles), (override));
    MOCK_METHOD(set<string>, nodes_exist, (const vector<string>& handles), (override));
    MOCK_METHOD(set<string>, links_exist, (const vector<string>& link_handles), (override));
   
    MOCK_METHOD(string, add_node, (const Node* node), (override));
    MOCK_METHOD(string, add_link, (const Link* link), (override));
    MOCK_METHOD(string, add_atom, (const Atom* atom), (override));

    MOCK_METHOD(vector<string>, add_atoms, (const vector<Atom*>& atoms), (override));
    MOCK_METHOD(vector<string>, add_nodes, (const vector<Node*>& nodes), (override));
    MOCK_METHOD(vector<string>, add_links, (const vector<Link*>& links), (override));

    MOCK_METHOD(bool, delete_atom, (const string& handle), (override));
    MOCK_METHOD(bool, delete_node, (const string& handle), (override));
    MOCK_METHOD(bool, delete_link, (const string& handle), (override));
    
    MOCK_METHOD(uint, delete_atoms, (const vector<string>& handles), (override));
    MOCK_METHOD(uint, delete_nodes, (const vector<string>& handles), (override));
    MOCK_METHOD(uint, delete_links, (const vector<string>& handles), (override));




    AtomDBMock() {
        ON_CALL(*this, get_atom_document(testing::_))
            .WillByDefault(::testing::Return(make_shared<MockAtomDocument>()));
        // testing::Mock::AllowLeak(this);
    }

   private:
    MOCK_METHOD(void, attention_broker_setup, (), (override));
};

class InferenceAgentTest : public ::testing::Test {
   protected:
    static void SetUpTestSuite() {
        GTEST_SKIP() << "Skipping";
        ServiceBusSingleton::provide(
            move(make_shared<MockServiceBus>("localhost:1111", "localhost:1121")));
        AtomDBSingleton::provide(move(make_shared<AtomDBMock>()));
        FitnessFunctionRegistry::initialize_statics();
    }
};

TEST_F(InferenceAgentTest, TestProofOfImplicationOrEquivalence) {
    auto inference_agent = new InferenceAgent();
    vector<string> tokens = {
        "PROOF_OF_IMPLICATION_OR_EQUIVALENCE", "handle1", "handle2", "1", "context"};
    auto inference_proxy = make_shared<MockInferenceProxy>(tokens);

    vector<string> calls_list = {"link_creation", "link_creation", "link_creation", "query_evolution"};
    int count = 0;
    auto mock_service_bus = dynamic_cast<MockServiceBus*>(ServiceBusSingleton::get_instance().get());
    // testing::Mock::AllowLeak(mock_service_bus);
    EXPECT_CALL(*mock_service_bus, issue_bus_command(testing::_))
        .Times(4)
        .WillRepeatedly(
            ::testing::Invoke([&calls_list, &count](shared_ptr<BusCommandProxy> command_proxy) {
                EXPECT_EQ(command_proxy->get_command(), calls_list[count]);
                count++;
            }));

    inference_agent->process_inference_request(inference_proxy);
    while (count < 4) {
        Utils::sleep(100);
    }
    inference_agent->stop();
    delete inference_agent;
}

TEST_F(InferenceAgentTest, TestProofOfImplication) {
    auto inference_agent = new InferenceAgent();
    vector<string> tokens = {"PROOF_OF_IMPLICATION", "handle1", "handle2", "1", "context"};
    auto inference_proxy = make_shared<MockInferenceProxy>(tokens);

    vector<string> calls_list = {"link_creation", "query_evolution"};
    int count = 0;
    int expected_calls = calls_list.size();
    auto mock_service_bus = dynamic_cast<MockServiceBus*>(ServiceBusSingleton::get_instance().get());
    // testing::Mock::AllowLeak(mock_service_bus);
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
    auto inference_agent = new InferenceAgent();
    vector<string> tokens = {"PROOF_OF_EQUIVALENCE", "handle1", "handle2", "1", "context"};
    auto inference_proxy = make_shared<MockInferenceProxy>(tokens);

    vector<string> calls_list = {"link_creation", "query_evolution"};
    int count = 0;
    int expected_calls = calls_list.size();
    auto mock_service_bus = dynamic_cast<MockServiceBus*>(ServiceBusSingleton::get_instance().get());
    // testing::Mock::AllowLeak(mock_service_bus);
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

TEST(InferenceAgentTest, TestInferenceRequests) {
    ProofOfImplicationOrEquivalence proof_of_implication_or_equivalence(
        "handle1", "handle2", 1, "context");
    auto requests = proof_of_implication_or_equivalence.get_requests();
    auto dic_request = proof_of_implication_or_equivalence.get_distributed_inference_control_request();
    EXPECT_EQ(requests.size(), 3);
    EXPECT_EQ(Utils::join(requests[0], ' '),
              "LINK_TEMPLATE Expression 3 NODE Symbol EVALUATION LINK_TEMPLATE Expression 2 NODE "
              "Symbol "
              "PREDICATE VARIABLE P LINK_TEMPLATE Expression 2 NODE Symbol CONCEPT VARIABLE C "
              "LIST 2 LINK_CREATE Expression 2 1 NODE Symbol SATISFYING_SET VARIABLE P CUSTOM_FIELD "
              "truth_value 2 strength 1.0 confidence 1.0 LINK_CREATE Expression 2 1 NODE Symbol "
              "PATTERNS VARIABLE C CUSTOM_FIELD truth_value 2 strength 1.0 confidence 1.0");
    EXPECT_EQ(Utils::join(dic_request, ' '),
              "OR 2 LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION NODE mocked_value mocked_value "
              "NODE mocked_value mocked_value LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE NODE "
              "mocked_value mocked_value NODE mocked_value mocked_value");

    ProofOfImplication proof_of_implication("handle3", "handle4", 2, "context");
    requests = proof_of_implication.get_requests();
    dic_request = proof_of_implication.get_distributed_inference_control_request();
    EXPECT_EQ(requests.size(), 1);
    EXPECT_EQ(Utils::join(requests[0], ' '),
              "AND 2 LINK_TEMPLATE Expression 2 NODE Symbol SATISFYING_SET VARIABLE P1 LINK_TEMPLATE "
              "Expression 2 NODE Symbol SATISFYING_SET VARIABLE P2 PROOF_OF_IMPLICATION");
    EXPECT_EQ(Utils::join(dic_request, ' '),
              "OR 6 AND 2 LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION NODE mocked_value "
              "mocked_value VARIABLE V1 LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION VARIABLE V1 "
              "NODE mocked_value mocked_value AND 2 LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE "
              "NODE mocked_value mocked_value VARIABLE V1 LINK_TEMPLATE Expression 3 NODE Symbol "
              "IMPLICATION VARIABLE V1 NODE mocked_value mocked_value AND 2 LINK_TEMPLATE Expression 3 "
              "NODE Symbol IMPLICATION NODE mocked_value mocked_value VARIABLE V1 LINK_TEMPLATE "
              "Expression 3 NODE Symbol EQUIVALENCE VARIABLE V1 NODE mocked_value mocked_value AND 2 "
              "LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE NODE mocked_value mocked_value "
              "VARIABLE V1 LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE VARIABLE V1 NODE "
              "mocked_value mocked_value LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION NODE "
              "mocked_value mocked_value NODE mocked_value mocked_value LINK_TEMPLATE Expression 3 NODE "
              "Symbol EQUIVALENCE NODE mocked_value mocked_value NODE mocked_value mocked_value");

    ProofOfEquivalence proof_of_equivalence("handle5", "handle6", 1, "context2");
    requests = proof_of_equivalence.get_requests();
    dic_request = proof_of_equivalence.get_distributed_inference_control_request();
    EXPECT_EQ(requests.size(), 1);
    EXPECT_EQ(Utils::join(requests[0], ' '),
              "AND 2 LINK_TEMPLATE Expression 2 NODE Symbol PATTERNS VARIABLE C1 LINK_TEMPLATE "
              "Expression 2 NODE Symbol PATTERNS VARIABLE C2 PROOF_OF_EQUIVALENCE");
    EXPECT_EQ(Utils::join(dic_request, ' '),
              "OR 2 LINK_TEMPLATE Expression 3 NODE Symbol IMPLICATION NODE mocked_value mocked_value "
              "NODE mocked_value mocked_value LINK_TEMPLATE Expression 3 NODE Symbol EQUIVALENCE NODE "
              "mocked_value mocked_value NODE mocked_value mocked_value");
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