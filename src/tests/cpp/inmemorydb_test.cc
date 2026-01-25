#include "InMemoryDB.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "Link.h"
#include "LinkSchema.h"
#include "Node.h"

using namespace atomdb;
using namespace atoms;
using namespace std;

class InMemoryDBTest : public ::testing::Test {
   protected:
    void SetUp() override { db = make_shared<InMemoryDB>("inmemorydb_test_"); }

    void TearDown() override {}

    shared_ptr<InMemoryDB> db;
};

TEST_F(InMemoryDBTest, AddNodesAndLinks) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto chimp = new Node("Symbol", "\"chimp\"");
    auto mammal = new Node("Symbol", "\"mammal\"");
    auto similarity = new Node("Symbol", "Similarity");
    auto inheritance = new Node("Symbol", "Inheritance");

    string human_handle = db->add_node(human, false);
    string monkey_handle = db->add_node(monkey, false);
    string chimp_handle = db->add_node(chimp, false);
    string mammal_handle = db->add_node(mammal, false);
    string similarity_handle = db->add_node(similarity, false);
    string inheritance_handle = db->add_node(inheritance, false);

    // Verify nodes were added
    EXPECT_TRUE(db->node_exists(human_handle));
    EXPECT_TRUE(db->node_exists(monkey_handle));
    EXPECT_TRUE(db->node_exists(chimp_handle));
    EXPECT_TRUE(db->node_exists(mammal_handle));
    EXPECT_TRUE(db->node_exists(similarity_handle));
    EXPECT_TRUE(db->node_exists(inheritance_handle));

    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    auto link2 = new Link("Expression", {similarity_handle, human_handle, chimp_handle});
    auto link3 = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});
    auto link4 = new Link("Expression", {inheritance_handle, monkey_handle, mammal_handle});
    auto link5 = new Link("Expression", {inheritance_handle, chimp_handle, mammal_handle});

    string link1_handle = db->add_link(link1, false);
    string link2_handle = db->add_link(link2, false);
    string link3_handle = db->add_link(link3, false);
    string link4_handle = db->add_link(link4, false);
    string link5_handle = db->add_link(link5, false);

    // Verify links were added
    EXPECT_TRUE(db->link_exists(link1_handle));
    EXPECT_TRUE(db->link_exists(link2_handle));
    EXPECT_TRUE(db->link_exists(link3_handle));
    EXPECT_TRUE(db->link_exists(link4_handle));
    EXPECT_TRUE(db->link_exists(link5_handle));

    // Verify we can retrieve atoms
    auto retrieved_human = db->get_atom(human_handle);
    EXPECT_EQ(retrieved_human->handle(), human_handle);

    auto retrieved_link1 = db->get_atom(link1_handle);
    EXPECT_EQ(retrieved_link1->handle(), link1_handle);
}

TEST_F(InMemoryDBTest, QueryForPattern) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto chimp = new Node("Symbol", "\"chimp\"");
    auto mammal = new Node("Symbol", "\"mammal\"");
    auto inheritance = new Node("Symbol", "Inheritance");

    string human_handle = db->add_node(human, false);
    string monkey_handle = db->add_node(monkey, false);
    string chimp_handle = db->add_node(chimp, false);
    string mammal_handle = db->add_node(mammal, false);
    string inheritance_handle = db->add_node(inheritance, false);

    auto link1 = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});
    auto link2 = new Link("Expression", {inheritance_handle, monkey_handle, mammal_handle});
    auto link3 = new Link("Expression", {inheritance_handle, chimp_handle, mammal_handle});

    string link1_handle = db->add_link(link1, false);
    string link2_handle = db->add_link(link2, false);
    string link3_handle = db->add_link(link3, false);

    // Re-index patterns to ensure re_index works
    db->re_index_patterns(true);

    LinkSchema link_schema({"LINK_TEMPLATE",
                            "Expression",
                            "3",
                            "NODE",
                            "Symbol",
                            "Inheritance",
                            "VARIABLE",
                            "x",
                            "NODE",
                            "Symbol",
                            "\"mammal\""});

    auto result = db->query_for_pattern(link_schema);
    EXPECT_EQ(result->size(), 3);

    // Verify we got the expected handles
    auto it = result->get_iterator();
    char* handle;
    vector<string> handles;
    while ((handle = it->next()) != nullptr) {
        handles.push_back(handle);
    }

    // Check that all three links are in the result
    EXPECT_TRUE(find(handles.begin(), handles.end(), link1_handle) != handles.end());
    EXPECT_TRUE(find(handles.begin(), handles.end(), link2_handle) != handles.end());
    EXPECT_TRUE(find(handles.begin(), handles.end(), link3_handle) != handles.end());
}

TEST_F(InMemoryDBTest, QueryForPatternWithSpecificMatch) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto similarity = new Node("Symbol", "Similarity");

    string human_handle = db->add_node(human, false);
    string monkey_handle = db->add_node(monkey, false);
    string similarity_handle = db->add_node(similarity, false);

    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});

    string link1_handle = db->add_link(link1, false);

    LinkSchema link_schema({"LINK_TEMPLATE",
                            "Expression",
                            "3",
                            "NODE",
                            "Symbol",
                            "Similarity",
                            "NODE",
                            "Symbol",
                            "\"human\"",
                            "VARIABLE",
                            "x"});

    auto result = db->query_for_pattern(link_schema);

    EXPECT_EQ(result->size(), 1);

    auto it = result->get_iterator();
    char* handle = it->next();
    EXPECT_EQ(string(handle), link1_handle);
}

TEST_F(InMemoryDBTest, QueryForPatternNoMatches) {
    LinkSchema link_schema({"LINK_TEMPLATE",
                            "Expression",
                            "3",
                            "NODE",
                            "Symbol",
                            "NonExistent",
                            "VARIABLE",
                            "x",
                            "VARIABLE",
                            "y"});

    auto result = db->query_for_pattern(link_schema);
    EXPECT_EQ(result->size(), 0);
}

TEST_F(InMemoryDBTest, QueryForTargets) {
    auto node1 = new Node("Symbol", "Node1");
    auto node2 = new Node("Symbol", "Node2");
    auto node3 = new Node("Symbol", "Node3");
    auto similarity = new Node("Symbol", "Similarity");

    string node1_handle = db->add_node(node1, false);
    string node2_handle = db->add_node(node2, false);
    string node3_handle = db->add_node(node3, false);
    string similarity_handle = db->add_node(similarity, false);

    auto node_targets = db->query_for_targets(node1_handle);
    EXPECT_EQ(node_targets, nullptr);

    auto link1 = new Link("Expression", {similarity_handle, node1_handle, node2_handle, node3_handle});
    string link1_handle = db->add_link(link1, false);

    auto link1_targets = db->query_for_targets(link1_handle);
    EXPECT_EQ(link1_targets->size(), 4);
    EXPECT_EQ(string(link1_targets->get_handle(0)), similarity_handle);
    EXPECT_EQ(string(link1_targets->get_handle(1)), node1_handle);
    EXPECT_EQ(string(link1_targets->get_handle(2)), node2_handle);
    EXPECT_EQ(string(link1_targets->get_handle(3)), node3_handle);
}

TEST_F(InMemoryDBTest, QueryForTargetsNonExistent) {
    string non_existent_handle = "00000000000000000000000000000000";
    auto targets = db->query_for_targets(non_existent_handle);
    EXPECT_EQ(targets, nullptr);
}

TEST_F(InMemoryDBTest, QueryForTargetsMultipleLinks) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto chimp = new Node("Symbol", "\"chimp\"");
    auto similarity = new Node("Symbol", "Similarity");

    string human_handle = db->add_node(human, false);
    string monkey_handle = db->add_node(monkey, false);
    string chimp_handle = db->add_node(chimp, false);
    string similarity_handle = db->add_node(similarity, false);

    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    auto link2 = new Link("Expression", {similarity_handle, human_handle, chimp_handle});
    auto link3 = new Link("Expression", {similarity_handle, monkey_handle, chimp_handle});

    string link1_handle = db->add_link(link1, false);
    string link2_handle = db->add_link(link2, false);
    string link3_handle = db->add_link(link3, false);

    auto link1_targets = db->query_for_targets(link1_handle);
    EXPECT_EQ(link1_targets->size(), 3);
    EXPECT_EQ(string(link1_targets->get_handle(0)), similarity_handle);
    EXPECT_EQ(string(link1_targets->get_handle(1)), human_handle);
    EXPECT_EQ(string(link1_targets->get_handle(2)), monkey_handle);

    auto link2_targets = db->query_for_targets(link2_handle);
    EXPECT_EQ(link2_targets->size(), 3);
    EXPECT_EQ(string(link2_targets->get_handle(0)), similarity_handle);
    EXPECT_EQ(string(link2_targets->get_handle(1)), human_handle);
    EXPECT_EQ(string(link2_targets->get_handle(2)), chimp_handle);

    auto link3_targets = db->query_for_targets(link3_handle);
    EXPECT_EQ(link3_targets->size(), 3);
    EXPECT_EQ(string(link3_targets->get_handle(0)), similarity_handle);
    EXPECT_EQ(string(link3_targets->get_handle(1)), monkey_handle);
    EXPECT_EQ(string(link3_targets->get_handle(2)), chimp_handle);
}

TEST_F(InMemoryDBTest, QueryForTargetsAfterDeletion) {
    auto node1 = new Node("Symbol", "Node1");
    auto node2 = new Node("Symbol", "Node2");
    auto similarity = new Node("Symbol", "Similarity");

    string node1_handle = db->add_node(node1, false);
    string node2_handle = db->add_node(node2, false);
    string similarity_handle = db->add_node(similarity, false);

    auto link1 = new Link("Expression", {similarity_handle, node1_handle, node2_handle});
    string link1_handle = db->add_link(link1, false);

    auto targets = db->query_for_targets(link1_handle);
    EXPECT_EQ(targets->size(), 3);

    db->delete_link(link1_handle, false);

    targets = db->query_for_targets(link1_handle);
    EXPECT_EQ(targets, nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
