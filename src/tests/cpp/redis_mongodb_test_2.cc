#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "AtomDBSingleton.h"
#include "Hasher.h"
#include "Link.h"
#include "MettaMapping.h"
#include "MockAnimalsData.h"
#include "Node.h"
#include "RedisMongoDB.h"
#include "TestConfig.h"

using namespace atomdb;
using namespace atoms;
using namespace std;

class RedisMongoDBTestEnvironment : public ::testing::Environment {
   public:
    void SetUp() override {
        TestConfig::load_environment();
        TestConfig::disable_atomdb_cache();
        auto atomdb = new RedisMongoDB("test2_");
        atomdb->drop_all();
        AtomDBSingleton::provide(shared_ptr<AtomDB>(atomdb));
    }

    void TearDown() override {
        auto atomdb = AtomDBSingleton::get_instance();
        auto db = dynamic_pointer_cast<RedisMongoDB>(atomdb);
        db->drop_all();
    }
};

class RedisMongoDBTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto atomdb = AtomDBSingleton::get_instance();
        db2 = dynamic_pointer_cast<RedisMongoDB>(atomdb);
        ASSERT_NE(db2, nullptr) << "Failed to cast AtomDB to RedisMongoDB (db2)";
    }

    void TearDown() override {}

    shared_ptr<RedisMongoDB> db2;
};

class LinkSchemaHandle : public LinkSchema {
   public:
    LinkSchemaHandle(const char* handle) : LinkSchema("blah", 2), fixed_handle(handle) {}

    string handle() const override { return this->fixed_handle; }

   private:
    string fixed_handle;
};

TEST_F(RedisMongoDBTest, AddLinksWithNoPatternIndexSchema) {
    // (TestLink (TestNestedLink1 (TestNestedLink2 N1 N2) N2) N2)
    vector<string> handles;

    auto link_node = new Node("Symbol", "TestLink");
    handles.push_back(db2->add_node(link_node));

    auto nested_link_node_1 = new Node("Symbol", "TestNestedLink1");
    handles.push_back(db2->add_node(nested_link_node_1));
    auto nested_link_node_2 = new Node("Symbol", "TestNestedLink2");
    handles.push_back(db2->add_node(nested_link_node_2));

    auto n1_node = new Node("Symbol", "N1");
    handles.push_back(db2->add_node(n1_node));
    auto n2_node = new Node("Symbol", "N2");
    handles.push_back(db2->add_node(n2_node));

    auto nested_link_2 =
        new Link("Expression", {nested_link_node_2->handle(), n1_node->handle(), n2_node->handle()});
    handles.push_back(db2->add_link(nested_link_2));

    auto nested_link_1 = new Link(
        "Expression", {nested_link_node_1->handle(), nested_link_2->handle(), n2_node->handle()});
    handles.push_back(db2->add_link(nested_link_1));

    auto link =
        new Link("Expression", {link_node->handle(), nested_link_1->handle(), n2_node->handle()});
    handles.push_back(db2->add_link(link));

    // (TestNestedLink2 * *)
    string hash = Hasher::link_handle(
        "Expression", {nested_link_node_2->handle(), Atom::WILDCARD_STRING, Atom::WILDCARD_STRING});
    auto link_schema = new LinkSchemaHandle(hash.c_str());
    auto handle_set = db2->query_for_pattern(*link_schema);
    EXPECT_EQ(handle_set->size(), 1);

    // (TestNestedLink1 * *)
    hash = Hasher::link_handle(
        "Expression", {nested_link_node_1->handle(), Atom::WILDCARD_STRING, Atom::WILDCARD_STRING});
    link_schema = new LinkSchemaHandle(hash.c_str());
    handle_set = db2->query_for_pattern(*link_schema);
    EXPECT_EQ(handle_set->size(), 1);

    // (TestLinkName * *)
    hash = Hasher::link_handle("Expression",
                               {link_node->handle(), Atom::WILDCARD_STRING, Atom::WILDCARD_STRING});
    link_schema = new LinkSchemaHandle(hash.c_str());
    handle_set = db2->query_for_pattern(*link_schema);
    EXPECT_EQ(handle_set->size(), 1);

    // (* * *)
    hash = Hasher::link_handle("Expression",
                               {Atom::WILDCARD_STRING, Atom::WILDCARD_STRING, Atom::WILDCARD_STRING});
    link_schema = new LinkSchemaHandle(hash.c_str());
    handle_set = db2->query_for_pattern(*link_schema);
    EXPECT_EQ(handle_set->size(), 3);

    // (* N1 *)
    hash = Hasher::link_handle("Expression",
                               {Atom::WILDCARD_STRING, n1_node->handle(), Atom::WILDCARD_STRING});
    link_schema = new LinkSchemaHandle(hash.c_str());
    handle_set = db2->query_for_pattern(*link_schema);
    EXPECT_EQ(handle_set->size(), 1);

    // (* * N2)
    hash = Hasher::link_handle("Expression",
                               {Atom::WILDCARD_STRING, Atom::WILDCARD_STRING, n2_node->handle()});
    link_schema = new LinkSchemaHandle(hash.c_str());
    handle_set = db2->query_for_pattern(*link_schema);
    EXPECT_EQ(handle_set->size(), 3);

    // Delete nested_link_1 means deleting link but not nested_link_2.
    EXPECT_TRUE(db2->delete_link(nested_link_1->handle()));
    EXPECT_FALSE(db2->link_exists(link->handle()));
    EXPECT_FALSE(db2->link_exists(nested_link_1->handle()));
    EXPECT_TRUE(db2->link_exists(nested_link_2->handle()));

    // Before delete: 5 nodes + 3 links
    // After delete: 5 nodes + 1 link (nested_link_2)
    EXPECT_EQ(db2->atoms_exist(handles).size(), 6);

    db2->delete_atoms(handles);
    EXPECT_EQ(db2->atoms_exist(handles).size(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new RedisMongoDBTestEnvironment());
    return RUN_ALL_TESTS();
}
