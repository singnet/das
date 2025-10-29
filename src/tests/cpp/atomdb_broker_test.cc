#include <chrono>
#include <iostream>
#include <memory>

#include "AtomDBProcessor.h"
#include "AtomDBProxy.h"
#include "AtomDBSingleton.h"
#include "AttentionBrokerClient.h"
#include "Hasher.h"
#include "PatternMatchingQueryProcessor.h"
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

    string timestamp() {
        auto now = std::chrono::system_clock::now();
        double seconds = std::chrono::duration<double>(now.time_since_epoch()).count();
        return std::to_string(seconds);
    }

    shared_ptr<RedisMongoDB> db;
};

TEST_F(AtomDBTest, AddAtoms) {
    string query_agent_id = "0.0.0.0:51000";
    string atomdb_broker_server_id = "0.0.0.0:51001";
    string atomdb_broker_client_id = "0.0.0.0:51002";

    ServiceBusSingleton::init(query_agent_id, atomdb_broker_server_id, 51003, 51999);

    shared_ptr<ServiceBus> service_bus = ServiceBusSingleton::get_instance();
    service_bus->register_processor(make_shared<PatternMatchingQueryProcessor>());
    Utils::sleep(500);

    shared_ptr<ServiceBus> atomdb_broker_server_bus =
        make_shared<ServiceBus>(atomdb_broker_server_id, query_agent_id);
    Utils::sleep(500);
    atomdb_broker_server_bus->register_processor(make_shared<AtomDBProcessor>());
    Utils::sleep(500);

    shared_ptr<ServiceBus> atomdb_broker_client_bus =
        make_shared<ServiceBus>(atomdb_broker_client_id, atomdb_broker_server_id);
    Utils::sleep(500);

    auto proxy = make_shared<AtomDBProxy>();

    atomdb_broker_client_bus->issue_bus_command(proxy);
    Utils::sleep(1000);

    string name1 = timestamp();
    string name2 = timestamp();
    string name3 = timestamp();

    string node1 = Hasher::node_handle("Symbol", name1);
    string node2 = Hasher::node_handle("Symbol", name2);
    string node3 = Hasher::node_handle("Symbol", name3);

    vector<string> tokens = {"NODE", "Symbol",     "false", "3", "is_literal", "bool", "false", name1,
                             "NODE", "Symbol",     "false", "3", "is_literal", "bool", "true",  name2,
                             "NODE", "Symbol",     "false", "3", "is_literal", "bool", "true",  name3,
                             "LINK", "Expression", "true",  "0", "3",          node1,  node2,   node3};

    auto link = new Link("Expression", {node1, node2, node3}, true);
    string link_handle = link->handle();

    auto atoms = proxy->build_atoms_from_tokens(tokens);

    EXPECT_TRUE(atoms[0]->handle() == node1);
    EXPECT_TRUE(atoms[1]->handle() == node2);
    EXPECT_TRUE(atoms[2]->handle() == node3);
    EXPECT_TRUE(atoms[3]->handle() == link_handle);

    EXPECT_FALSE(db->node_exists(node1));
    EXPECT_FALSE(db->node_exists(node2));
    EXPECT_FALSE(db->node_exists(node3));
    EXPECT_FALSE(db->link_exists(link_handle));

    vector<string> handles = proxy->add_atoms(atoms);

    EXPECT_TRUE(handles[0] == node1);
    EXPECT_TRUE(handles[1] == node2);
    EXPECT_TRUE(handles[2] == node3);
    EXPECT_TRUE(handles[3] == link_handle);

    Utils::sleep(2000);

    EXPECT_TRUE(db->node_exists(node1));
    EXPECT_TRUE(db->node_exists(node2));
    EXPECT_TRUE(db->node_exists(node3));
    EXPECT_TRUE(db->link_exists(link_handle));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new AtomDBTestEnvironment());
    return RUN_ALL_TESTS();
}
