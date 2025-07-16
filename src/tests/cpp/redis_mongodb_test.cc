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
#include "Node.h"
#include "RedisMongoDB.h"

using namespace atomdb;
using namespace atoms;
using namespace std;

class MockDecoder : public HandleDecoder {
   public:
    map<string, shared_ptr<Atom>> atoms;
    shared_ptr<Atom> get_atom(const string& handle) { return this->atoms[handle]; }
    shared_ptr<Atom> add_atom(shared_ptr<Atom> atom) {
        this->atoms[atom->handle()] = atom;
        return atom;
    }
};

class RedisMongoDBTestEnvironment : public ::testing::Environment {
   public:
    void SetUp() override {
        setenv("DAS_REDIS_HOSTNAME", "localhost", 1);
        setenv("DAS_REDIS_PORT", "29000", 1);
        setenv("DAS_USE_REDIS_CLUSTER", "false", 1);
        setenv("DAS_MONGODB_HOSTNAME", "localhost", 1);
        setenv("DAS_MONGODB_PORT", "28000", 1);
        setenv("DAS_MONGODB_USERNAME", "dbadmin", 1);
        setenv("DAS_MONGODB_PASSWORD", "dassecret", 1);
        setenv("DAS_DISABLE_ATOMDB_CACHE", "true", 1);
        AtomDBSingleton::init();
    }
};

class RedisMongoDBTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto atomdb = AtomDBSingleton::get_instance();
        db = dynamic_pointer_cast<RedisMongoDB>(atomdb);
        ASSERT_NE(db, nullptr) << "Failed to cast AtomDB to RedisMongoDB";
    }

    void TearDown() override {}

    string exp_hash(vector<string> targets) {
        char* symbol = (char*) "Symbol";
        char** targets_handles = new char*[targets.size()];
        for (size_t i = 0; i < targets.size(); i++) {
            targets_handles[i] = terminal_hash(symbol, (char*) targets[i].c_str());
        }
        char* expression = named_type_hash((char*) "Expression");
        return string(expression_hash(expression, targets_handles, targets.size()));
    }

    shared_ptr<RedisMongoDB> db;
};

class LinkSchemaHandle : public LinkSchema {
   public:
    LinkSchemaHandle(const char* handle) : LinkSchema("blah", 2), fixed_handle(handle) {}

    string handle() const override { return this->fixed_handle; }

   private:
    string fixed_handle;
};

TEST_F(RedisMongoDBTest, DeleteNodesAndLinks) {
    vector<atoms::Node*> nodes;
    vector<atoms::Link*> links;
    MockDecoder decoder;

    auto similarity_node = new Node("Symbol", "Similarity");
    for (int i = 0; i < 55; i++) {
        auto test_1_node = new Node("Symbol", "add-links-1-" + to_string(i));
        auto test_2_node = new Node("Symbol", "add-links-2-" + to_string(i));
        nodes.push_back(test_1_node);
        nodes.push_back(test_2_node);
        links.push_back(new Link(
            "Expression", {similarity_node->handle(), test_1_node->handle(), test_2_node->handle()}));
    }

    auto nodes_handles = db->add_nodes(nodes);
    EXPECT_EQ(nodes_handles.size(), nodes.size());

    auto nodes_exist = db->nodes_exist(nodes_handles);
    EXPECT_EQ(nodes_exist.size(), nodes.size());

    auto links_handles = db->add_links(links);
    EXPECT_EQ(links_handles.size(), links.size());

    auto links_exist = db->links_exist(links_handles);
    EXPECT_EQ(links_exist.size(), links.size());

    EXPECT_EQ(db->delete_links(links_handles), links.size());
    // Deleting nodes first will delete the links (via incoming set deletion, as nodes are referenced by
    // links).
    EXPECT_EQ(db->delete_nodes(nodes_handles), nodes.size());

    auto nodes_exist_after_delete = db->nodes_exist(nodes_handles);
    EXPECT_EQ(nodes_exist_after_delete.size(), 0);

    auto links_exist_after_delete = db->links_exist(links_handles);
    EXPECT_EQ(links_exist_after_delete.size(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new RedisMongoDBTestEnvironment());
    return RUN_ALL_TESTS();
}
