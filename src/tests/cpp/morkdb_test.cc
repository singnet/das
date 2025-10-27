#include "MorkDB.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "AtomDBSingleton.h"
#include "LinkSchema.h"

using namespace atomdb;
using namespace atoms;
using namespace std;

class MorkDBTestEnvironment : public ::testing::Environment {
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
        setenv("DAS_MORK_HOSTNAME", "localhost", 1);
        setenv("DAS_MORK_PORT", "8000", 1);

        auto atomdb = new MorkDB("morkdb_test_");
        atomdb->drop_all();
        AtomDBSingleton::provide(shared_ptr<AtomDB>(atomdb));
    }
};

class MorkDBTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto atomdb = AtomDBSingleton::get_instance();
        db = dynamic_pointer_cast<MorkDB>(atomdb);
        ASSERT_NE(db, nullptr) << "Failed to cast AtomDB to MorkDB";
    }

    void TearDown() override {
        auto atomdb = AtomDBSingleton::get_instance();
        auto db = dynamic_pointer_cast<MorkDB>(atomdb);
        db->drop_all();
    }

    string exp_hash(vector<string> targets) {
        char* symbol = (char*) "Symbol";
        char** targets_handles = new char*[targets.size()];
        for (size_t i = 0; i < targets.size(); i++) {
            targets_handles[i] = terminal_hash(symbol, (char*) targets[i].c_str());
        }
        char* expression = named_type_hash((char*) "Expression");
        return string(expression_hash(expression, targets_handles, targets.size()));
    }

    shared_ptr<MorkDB> db;
};

static string expression = "Expression";
static string symbol = "Symbol";
static string link_template = "LINK_TEMPLATE";
static string node = "NODE";
static string variable = "VARIABLE";
static string inheritance = "Inheritance";
static string similarity = "Similarity";
static string human = "\"human\"";
static string monkey = "\"monkey\"";
static string chimp = "\"chimp\"";
static string rhino = "\"rhino\"";
static string mammal = "\"mammal\"";

TEST_F(MorkDBTest, QueryForPattern) {
    string handle_1 = exp_hash({inheritance, human, mammal});
    string handle_2 = exp_hash({inheritance, monkey, mammal});
    string handle_3 = exp_hash({inheritance, chimp, mammal});
    string handle_4 = exp_hash({inheritance, rhino, mammal});

    // clang-format off
    LinkSchema link_schema({
        link_template, expression, "3", 
            node, symbol, inheritance, 
            variable, "$x",
            node, symbol, mammal});
    // clang-format on

    auto result = db->query_for_pattern(link_schema);

    ASSERT_EQ(result->size(), 4);

    auto it = result->get_iterator();
    char* handle;
    vector<string> handles;
    while ((handle = it->next()) != nullptr) {
        handles.push_back(handle);
    }

    vector<string> expected_handles = {handle_1, handle_2, handle_3, handle_4};

    std::sort(handles.begin(), handles.end());
    std::sort(expected_handles.begin(), expected_handles.end());

    ASSERT_EQ(handles, expected_handles);
}

TEST_F(MorkDBTest, QueryForTargets) {
    string link_handle = exp_hash({"Inheritance", "\"human\"", "\"mammal\""});
    EXPECT_EQ(db->query_for_targets(link_handle), nullptr);
}

TEST_F(MorkDBTest, ConcurrentQueryForPattern) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    auto worker = [&](int thread_id) {
        try {
            // clang-format off
            LinkSchema link_schema({
                link_template, expression, "3", 
                    node, symbol, similarity, 
                    variable, "$x",
                    variable, "$y"});
            // clang-format on
            auto handle_set = db->query_for_pattern(link_schema);
            ASSERT_NE(handle_set, nullptr);
            ASSERT_EQ(handle_set->size(), 14);
            success_count++;
        } catch (const exception& e) {
            cout << "Thread " << thread_id << " failed with error: " << e.what() << endl;
        }
    };

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count, num_threads);

    // // Test non-existing pattern
    // clang-format off
    LinkSchema link_schema({
        link_template, expression, "3", 
            node, symbol, "Fake", 
            variable, "$x",
            variable, "$y"});
    // clang-format on
    auto handle_set = db->query_for_pattern(link_schema);
    EXPECT_EQ(handle_set->size(), 0);
}

TEST_F(MorkDBTest, AddGetAndDeleteNode) {
    auto node = new Node("Symbol", "TestNode");
    auto node_handle = db->add_node(node);

    auto node_document = db->get_atom_document(node_handle);
    EXPECT_EQ(string(node_document->get("named_type")), string("Symbol"));
    EXPECT_EQ(string(node_document->get("name")), string("TestNode"));

    ASSERT_TRUE(db->delete_node(node_handle));
}

TEST_F(MorkDBTest, AddGetAndDeleteNodes) {
    vector<Node*> nodes;
    for (int i = 0; i < 10; i++) {
        nodes.push_back(new Node("Symbol", "TestNode" + to_string(i)));
    }

    auto nodes_handles = db->add_nodes(nodes);
    EXPECT_EQ(nodes.size(), nodes.size());

    auto nodes_documents = db->get_atom_documents(nodes_handles, {"_id"});
    EXPECT_EQ(nodes_documents.size(), nodes.size());

    EXPECT_EQ(db->delete_nodes(nodes_handles), nodes.size());

    ASSERT_EQ(db->nodes_exist(nodes_handles).size(), 0);
}

TEST_F(MorkDBTest, AddGetAndDeleteLink) {
    auto similarity_node = new Node("Symbol", "Similarity");
    auto similarity_node_handle = db->add_node(similarity_node);

    auto node1 = new Node("Symbol", "Node1");
    auto node1_handle = db->add_node(node1);
    auto node2 = new Node("Symbol", "Node2");
    auto node2_handle = db->add_node(node2);
    auto node3 = new Node("Symbol", "Node3");
    auto node3_handle = db->add_node(node3);
    auto node4 = new Node("Symbol", "Node4");
    auto node4_handle = db->add_node(node4);

    ASSERT_TRUE(db->node_exists(similarity_node_handle));
    ASSERT_TRUE(db->node_exists(node1_handle));
    ASSERT_TRUE(db->node_exists(node2_handle));
    ASSERT_TRUE(db->node_exists(node3_handle));
    ASSERT_TRUE(db->node_exists(node4_handle));

    auto link = new Link("Expression", {similarity_node_handle, node1_handle, node2_handle});
    auto link_handle = db->add_link(link);

    ASSERT_TRUE(db->link_exists(link_handle));

    auto link_document = db->get_atom_document(link_handle);
    EXPECT_EQ(string(link_document->get("named_type")), string("Expression"));
    EXPECT_EQ(string(link_document->get("targets", 0)), string(similarity_node_handle));
    EXPECT_EQ(string(link_document->get("targets", 1)), string(node1_handle));
    EXPECT_EQ(string(link_document->get("targets", 2)), string(node2_handle));

    // clang-format off
    LinkSchema link_schema({
        link_template, expression, "3", 
            node, symbol, "Similarity", 
            node, symbol, "Node1",
            variable, "S"});
    // clang-format on

    auto handle_set = db->query_for_pattern(link_schema);
    EXPECT_EQ(handle_set->size(), 1);

    auto link_13 = new Link("Expression", {similarity_node_handle, node1_handle, node3_handle});
    auto link_14 = new Link("Expression", {similarity_node_handle, node1_handle, node4_handle});

    auto new_link_handles = db->add_links({link_13, link_14});
    EXPECT_EQ(new_link_handles.size(), 2);

    handle_set = db->query_for_pattern(link_schema);
    EXPECT_EQ(handle_set->size(), 3);

    // MORKDB does not support deleting links, so it must always return false
    EXPECT_FALSE(db->delete_link(link_handle, true));
}

TEST_F(MorkDBTest, AddGetAndDeleteLinks) {
    auto similarity_node = new Node("Symbol", "Similarity");
    auto similarity_node_handle = db->add_node(similarity_node);

    auto from_node = new Node("Symbol", "From");
    auto from_node_handle = db->add_node(from_node);

    vector<Link*> links;
    for (int i = 0; i < 10; i++) {
        auto to_node = new Node("Symbol", "To-" + to_string(i));
        auto to_node_handle = db->add_node(to_node);
        links.push_back(
            new Link("Expression", {similarity_node_handle, from_node_handle, to_node_handle}));
    }

    auto links_handles = db->add_links(links);
    EXPECT_EQ(links_handles.size(), links.size());

    auto links_documents = db->get_atom_documents(links_handles, {"_id"});
    EXPECT_EQ(links_documents.size(), links.size());

    // MORKDB does not support deleting links, so it must always return false
    EXPECT_FALSE(db->delete_links(links_handles, true));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new MorkDBTestEnvironment());
    return RUN_ALL_TESTS();
}
