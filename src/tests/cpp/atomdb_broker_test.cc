#include <memory>

#include "AtomDBSingleton.h"
#include "AtomDBProcessor.h"
#include "PatternMatchingQueryProcessor.h"
#include "AttentionBrokerClient.h"
#include "AtomDBProxy.h"
#include "Hasher.h"
#include "RedisMongoDB.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"
#include "TestConfig.h"
#include "Utils.h"
#include "gtest/gtest.h"
#include "test_utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace attention_broker;
using namespace atomdb_broker;
using namespace query_engine;


class AtomDBTestEnvironment : public ::testing::Environment {
   public:
    void SetUp() override {
        TestConfig::load_environment();
        AtomDBSingleton::init();
    }

    void TearDown() override {}
};

class AtomDBTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto atomdb = AtomDBSingleton::get_instance();
        db = dynamic_pointer_cast<RedisMongoDB>(atomdb);
        ASSERT_NE(db, nullptr) << "Failed to cast AtomDB to RedisMongoDB";
    }

    void TearDown() override {}

    shared_ptr<RedisMongoDB> db;
};

TEST_F(AtomDBTest, AddAtoms) {
    string query_agent_id = "0.0.0.0:42000";
    string atomdb_broker_server_id = "0.0.0.0:42010";
    string atomdb_broker_client_id = "0.0.0.0:42020";

    AttentionBrokerClient::set_server_address("0.0.0.0:37007");

    ServiceBusSingleton::init(query_agent_id, "0.0.0.0:37007", 42000, 42999);
    shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
    service_bus->register_processor(make_shared<PatternMatchingQueryProcessor>());
    Utils::sleep(500);

    shared_ptr<ServiceBus> atomdb_broker_server_bus = make_shared<ServiceBus>(atomdb_broker_server_id, query_agent_id);
    Utils::sleep(500);
    atomdb_broker_server_bus->register_processor(make_shared<AtomDBProcessor>());
    Utils::sleep(500);

    shared_ptr<ServiceBus> atomdb_broker_client_bus = make_shared<ServiceBus>(atomdb_broker_client_id, atomdb_broker_server_id);
    Utils::sleep(500);

    auto proxy = make_shared<AtomDBProxy>();
    
    atomdb_broker_client_bus->issue_bus_command(proxy);
    Utils::sleep(1000);

    string similarityB = Hasher::node_handle("Symbol", "SimilarityB");
    string humanB = Hasher::node_handle("Symbol", "\"humanB\"");
    string monkeyB = Hasher::node_handle("Symbol", "\"monkeyB\"");

    vector<string> tokens = {
        "NODE", "Symbol", "false", "3", "is_literal", "bool", "false", "SimilarityB",
        "NODE", "Symbol", "false", "3", "is_literal", "bool", "true", R"("humanB")",
        "NODE", "Symbol", "false", "3", "is_literal", "bool", "true", R"("monkeyB")",
        "LINK", "Expression", "true", "0", "3", similarityB, humanB, monkeyB
    };

    auto link = new Link("Expression", {similarityB, humanB, monkeyB}, true);
    string link_handle = link->handle();

    auto atoms = proxy->build_atoms_from_tokens(tokens);

    EXPECT_TRUE(atoms[0]->handle() == similarityB);
    EXPECT_TRUE(atoms[1]->handle() == humanB);
    EXPECT_TRUE(atoms[2]->handle() == monkeyB);
    EXPECT_TRUE(atoms[3]->handle() == link_handle);

    EXPECT_FALSE(db->node_exists(similarityB));
    EXPECT_FALSE(db->node_exists(humanB));
    EXPECT_FALSE(db->node_exists(monkeyB));
    EXPECT_FALSE(db->link_exists(link_handle));

    vector<string> handles = proxy->add_atoms(atoms);
        
    EXPECT_TRUE(handles[0] == similarityB);
    EXPECT_TRUE(handles[1] == humanB);
    EXPECT_TRUE(handles[2] == monkeyB);
    EXPECT_TRUE(handles[3] == link_handle);

    while (!proxy->finished()) {
        Utils::sleep();
    }

    EXPECT_TRUE(db->node_exists(similarityB));
    EXPECT_TRUE(db->node_exists(humanB));
    EXPECT_TRUE(db->node_exists(monkeyB));
    EXPECT_TRUE(db->link_exists(link_handle));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new AtomDBTestEnvironment());
    return RUN_ALL_TESTS();
}
