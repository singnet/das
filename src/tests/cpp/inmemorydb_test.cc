#include "InMemoryDB.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Assignment.h"
#include "InMemoryDBAPITypes.h"
#include "Link.h"
#include "LinkSchema.h"
#include "Node.h"
#include "Properties.h"

using namespace atomdb;
using namespace atomdb::atomdb_api_types;
using namespace atoms;
using namespace commons;
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

TEST_F(InMemoryDBTest, QueryForIncomingSet) {
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

    // Create links that reference human
    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    auto link2 = new Link("Expression", {similarity_handle, human_handle, chimp_handle});
    auto link3 = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});

    string link1_handle = db->add_link(link1, false);
    string link2_handle = db->add_link(link2, false);
    string link3_handle = db->add_link(link3, false);

    // Query incoming set for human
    auto incoming_set = db->query_for_incoming_set(human_handle);
    EXPECT_EQ(incoming_set->size(), 3);

    // Verify we got the expected link handles
    auto it = incoming_set->get_iterator();
    char* handle;
    vector<string> handles;
    while ((handle = it->next()) != nullptr) {
        handles.push_back(handle);
    }

    EXPECT_TRUE(find(handles.begin(), handles.end(), link1_handle) != handles.end());
    EXPECT_TRUE(find(handles.begin(), handles.end(), link2_handle) != handles.end());
    EXPECT_TRUE(find(handles.begin(), handles.end(), link3_handle) != handles.end());

    // Query incoming set for monkey (should have 1 link)
    auto monkey_incoming = db->query_for_incoming_set(monkey_handle);
    EXPECT_EQ(monkey_incoming->size(), 1);

    // Query incoming set for mammal (should have 1 link)
    auto mammal_incoming = db->query_for_incoming_set(mammal_handle);
    EXPECT_EQ(mammal_incoming->size(), 1);

    // Query incoming set for non-existent node (should be empty)
    string non_existent_handle = "00000000000000000000000000000000";
    auto non_existent_incoming = db->query_for_incoming_set(non_existent_handle);
    EXPECT_EQ(non_existent_incoming->size(), 0);
}

TEST_F(InMemoryDBTest, QueryForIncomingSetAfterDeletion) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto similarity = new Node("Symbol", "Similarity");

    string human_handle = db->add_node(human, false);
    string monkey_handle = db->add_node(monkey, false);
    string similarity_handle = db->add_node(similarity, false);

    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link1_handle = db->add_link(link1, false);

    // Verify incoming set before deletion
    auto incoming_set = db->query_for_incoming_set(human_handle);
    EXPECT_EQ(incoming_set->size(), 1);

    // Delete the link
    db->delete_link(link1_handle, false);

    // Verify incoming set is now empty
    incoming_set = db->query_for_incoming_set(human_handle);
    EXPECT_EQ(incoming_set->size(), 0);
}

TEST_F(InMemoryDBTest, DeleteAtom) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto similarity = new Node("Symbol", "Similarity");

    string human_handle = db->add_node(human, false);
    string monkey_handle = db->add_node(monkey, false);
    string similarity_handle = db->add_node(similarity, false);

    // Create a link that references human
    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link1_handle = db->add_link(link1, false);

    // Try to delete human atom with delete_link_targets=false (should fail)
    bool deleted = db->delete_atom(human_handle, false);
    EXPECT_FALSE(deleted);

    // Verify human still exists
    EXPECT_TRUE(db->node_exists(human_handle));
    EXPECT_TRUE(db->link_exists(link1_handle));

    // Create a link that references human
    auto link2 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link2_handle = db->add_link(link2, false);

    // Delete human atom with delete_link_targets=true (should succeed and delete the link)
    deleted = db->delete_atom(human_handle, true);
    EXPECT_TRUE(deleted);

    // Verify human is deleted
    EXPECT_FALSE(db->node_exists(human_handle));

    // Verify the link is also deleted
    EXPECT_FALSE(db->link_exists(link2_handle));
}

TEST_F(InMemoryDBTest, DeleteNode) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto similarity = new Node("Symbol", "Similarity");

    string human_handle = db->add_node(human, false);
    string monkey_handle = db->add_node(monkey, false);
    string similarity_handle = db->add_node(similarity, false);

    // Create a link that references human
    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link1_handle = db->add_link(link1, false);

    // Try to delete human with delete_link_targets=false (should fail)
    bool deleted = db->delete_node(human_handle, false);
    EXPECT_FALSE(deleted);

    // Verify human still exists
    EXPECT_TRUE(db->node_exists(human_handle));
    EXPECT_TRUE(db->link_exists(link1_handle));

    // Verify incoming set still has the link
    auto incoming_set = db->query_for_incoming_set(human_handle);
    EXPECT_EQ(incoming_set->size(), 1);

    // Create a link that references human
    auto link2 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link2_handle = db->add_link(link2, false);

    // Delete human with delete_link_targets=true (should succeed and delete the link)
    deleted = db->delete_node(human_handle, true);
    EXPECT_TRUE(deleted);

    // Verify human is deleted
    EXPECT_FALSE(db->node_exists(human_handle));

    // Verify the link is also deleted
    EXPECT_FALSE(db->link_exists(link2_handle));

    // Verify incoming set is empty
    incoming_set = db->query_for_incoming_set(human_handle);
    EXPECT_EQ(incoming_set->size(), 0);
}

TEST_F(InMemoryDBTest, DeleteLink) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto similarity = new Node("Symbol", "Similarity");

    string human_handle = db->add_node(human, false);
    string monkey_handle = db->add_node(monkey, false);
    string similarity_handle = db->add_node(similarity, false);

    // Create a link that references human and monkey
    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link1_handle = db->add_link(link1, false);

    // Delete link with delete_link_targets=false (should succeed, targets remain)
    bool deleted = db->delete_link(link1_handle, false);
    EXPECT_TRUE(deleted);

    // Verify link is deleted
    EXPECT_FALSE(db->link_exists(link1_handle));

    // Verify targets still exist
    EXPECT_TRUE(db->node_exists(human_handle));
    EXPECT_TRUE(db->node_exists(monkey_handle));
    EXPECT_TRUE(db->node_exists(similarity_handle));

    // Verify incoming sets are empty
    auto human_incoming = db->query_for_incoming_set(human_handle);
    EXPECT_EQ(human_incoming->size(), 0);
    auto monkey_incoming = db->query_for_incoming_set(monkey_handle);
    EXPECT_EQ(monkey_incoming->size(), 0);

    // Create a link that references human and monkey
    auto link2 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link2_handle = db->add_link(link2, false);

    // Delete link with delete_link_targets=true (should delete targets if no other references)
    deleted = db->delete_link(link2_handle, true);
    EXPECT_TRUE(deleted);

    // Verify link is deleted
    EXPECT_FALSE(db->link_exists(link2_handle));

    // Verify targets are deleted (they had no other incoming links)
    EXPECT_FALSE(db->node_exists(similarity_handle));
    EXPECT_FALSE(db->node_exists(human_handle));
    EXPECT_FALSE(db->node_exists(monkey_handle));
}

TEST_F(InMemoryDBTest, DeleteLinkMultipleReferences) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto chimp = new Node("Symbol", "\"chimp\"");
    auto similarity = new Node("Symbol", "Similarity");

    string human_handle = db->add_node(human, false);
    string monkey_handle = db->add_node(monkey, false);
    string chimp_handle = db->add_node(chimp, false);
    string similarity_handle = db->add_node(similarity, false);

    // Create two links that both reference human
    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    auto link2 = new Link("Expression", {similarity_handle, human_handle, chimp_handle});
    string link1_handle = db->add_link(link1, false);
    string link2_handle = db->add_link(link2, false);

    // Verify human has 2 incoming links
    auto human_incoming = db->query_for_incoming_set(human_handle);
    EXPECT_EQ(human_incoming->size(), 2);

    // Delete link1 with delete_link_targets=true
    bool deleted = db->delete_link(link1_handle, true);
    EXPECT_TRUE(deleted);

    // Verify link1 is deleted
    EXPECT_FALSE(db->link_exists(link1_handle));

    // Verify human still exists (has another incoming link)
    EXPECT_TRUE(db->node_exists(human_handle));

    // Verify monkey is deleted (no other references)
    EXPECT_FALSE(db->node_exists(monkey_handle));

    // Verify human now has only 1 incoming link
    human_incoming = db->query_for_incoming_set(human_handle);
    EXPECT_EQ(human_incoming->size(), 1);

    // Verify link2 still exists
    EXPECT_TRUE(db->link_exists(link2_handle));
}

TEST_F(InMemoryDBTest, AtomsCount) {
    EXPECT_EQ(db->atom_count(), 0);
    EXPECT_EQ(db->empty(), true);

    auto node1 = new Node("Symbol", "Node1");
    auto node2 = new Node("Symbol", "Node2");
    auto similarity = new Node("Symbol", "Similarity");

    db->add_node(node1, false);
    db->add_node(node2, false);
    db->add_node(similarity, false);

    EXPECT_EQ(db->atom_count(), 3);

    auto link1 = new Link("Expression", {similarity->handle(), node1->handle(), node2->handle()});
    db->add_link(link1, false);

    EXPECT_EQ(db->atom_count(), 4);
    EXPECT_EQ(db->empty(), false);
}

TEST_F(InMemoryDBTest, GetAllAtoms) {
    auto node1 = new Node("Symbol", "Node1");
    auto node2 = new Node("Symbol", "Node2");
    auto similarity = new Node("Symbol", "Similarity");

    string node1_handle = db->add_node(node1, false);
    string node2_handle = db->add_node(node2, false);
    string similarity_handle = db->add_node(similarity, false);

    auto link = new Link("Expression", {similarity_handle, node1_handle, node2_handle});
    string link_handle = db->add_link(link, false);

    auto atoms = db->get_all_atoms();
    EXPECT_EQ(atoms.size(), 4u);

    set<string> handles;
    for (const auto& atom : atoms) {
        handles.insert(atom->handle());
    }
    EXPECT_TRUE(handles.count(node1_handle) > 0);
    EXPECT_TRUE(handles.count(node2_handle) > 0);
    EXPECT_TRUE(handles.count(similarity_handle) > 0);
    EXPECT_TRUE(handles.count(link_handle) > 0);
}

TEST_F(InMemoryDBTest, GetAllAtomsReturnsIndependentClones) {
    auto node = new Node("Symbol", "Node1");
    string handle = db->add_node(node, false);

    auto atoms = db->get_all_atoms();
    ASSERT_EQ(atoms.size(), 1u);
    EXPECT_EQ(atoms[0]->handle(), handle);

    db->drop_all();

    // Clones returned earlier remain valid after the DB is cleared.
    EXPECT_EQ(atoms[0]->handle(), handle);
    EXPECT_EQ(db->get_atom(handle), nullptr);
}

TEST_F(InMemoryDBTest, DropAllClearsAtomsAndIndexes) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto mammal = new Node("Symbol", "\"mammal\"");
    auto inheritance = new Node("Symbol", "Inheritance");

    string human_handle = db->add_node(human, false);
    string monkey_handle = db->add_node(monkey, false);
    string mammal_handle = db->add_node(mammal, false);
    string inheritance_handle = db->add_node(inheritance, false);

    auto link1 = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});
    auto link2 = new Link("Expression", {inheritance_handle, monkey_handle, mammal_handle});
    string link1_handle = db->add_link(link1, false);
    string link2_handle = db->add_link(link2, false);

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

    EXPECT_EQ(db->query_for_pattern(link_schema)->size(), 2);
    EXPECT_EQ(db->query_for_incoming_set(human_handle)->size(), 1);
    EXPECT_NE(db->query_for_targets(link1_handle), nullptr);

    db->drop_all();

    EXPECT_EQ(db->atom_count(), 0u);
    EXPECT_EQ(db->empty(), true);
    EXPECT_EQ(db->get_atom(human_handle), nullptr);
    EXPECT_EQ(db->get_atom(link1_handle), nullptr);
    EXPECT_EQ(db->query_for_pattern(link_schema)->size(), 0);
    EXPECT_EQ(db->query_for_incoming_set(human_handle)->size(), 0);
    EXPECT_EQ(db->query_for_targets(link1_handle), nullptr);
}

TEST_F(InMemoryDBTest, DropAllOnEmptyDatabase) {
    EXPECT_EQ(db->atom_count(), 0u);
    EXPECT_EQ(db->get_all_atoms().size(), 0u);
    db->drop_all();
    EXPECT_EQ(db->atom_count(), 0u);
}

// =============================================================================
// HandleSetInMemory / HandleSetInMemoryIterator tests
//
// These cover the metadata-preserving add_handle overload and the iterator
// pointer-lifetime contract (next() returns a pointer owned by the HandleSet,
// not the iterator), used when federating nested-indexing backends.
// =============================================================================

TEST(HandleSetInMemoryTest, MetadataRoundTrip) {
    HandleSetInMemory set;

    const string handle = "0123456789abcdef0123456789abcdef";
    map<string, string> metta = {{"$x", "(Symbol human)"}, {"$y", "(Symbol mammal)"}};
    Assignment assignment;
    assignment.assign("$x", "human_value");
    assignment.assign("$y", "mammal_value");

    set.add_handle(handle, metta, assignment);

    EXPECT_EQ(set.size(), 1u);

    auto retrieved_metta = set.get_metta_expressions_by_handle(handle);
    EXPECT_EQ(retrieved_metta.size(), 2u);
    EXPECT_EQ(retrieved_metta["$x"], "(Symbol human)");
    EXPECT_EQ(retrieved_metta["$y"], "(Symbol mammal)");

    auto retrieved_assignment = set.get_assignments_by_handle(handle);
    EXPECT_EQ(retrieved_assignment.get("$x"), "human_value");
    EXPECT_EQ(retrieved_assignment.get("$y"), "mammal_value");
}

TEST(HandleSetInMemoryTest, MetadataAbsentForPlainAddHandle) {
    HandleSetInMemory set;
    const string handle = "0123456789abcdef0123456789abcdef";

    set.add_handle(handle);

    EXPECT_EQ(set.size(), 1u);
    // Plain add_handle stores no metadata; accessors must return empty (never throw).
    EXPECT_TRUE(set.get_metta_expressions_by_handle(handle).empty());
    EXPECT_EQ(set.get_assignments_by_handle(handle).variable_count(), 0u);
}

TEST(HandleSetInMemoryTest, AppendPreservesMetadataFromBothSets) {
    const string handle_a = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    const string handle_b = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";

    auto first = make_shared<HandleSetInMemory>();
    Assignment assignment_a;
    assignment_a.assign("$a", "value_a");
    first->add_handle(handle_a, {{"$a", "(Symbol a)"}}, assignment_a);

    auto second = make_shared<HandleSetInMemory>();
    Assignment assignment_b;
    assignment_b.assign("$b", "value_b");
    second->add_handle(handle_b, {{"$b", "(Symbol b)"}}, assignment_b);

    first->append(second);

    EXPECT_EQ(first->size(), 2u);

    // Metadata from the original set survives.
    EXPECT_EQ(first->get_metta_expressions_by_handle(handle_a)["$a"], "(Symbol a)");
    EXPECT_EQ(first->get_assignments_by_handle(handle_a).get("$a"), "value_a");

    // Metadata merged in from the appended set survives.
    EXPECT_EQ(first->get_metta_expressions_by_handle(handle_b)["$b"], "(Symbol b)");
    EXPECT_EQ(first->get_assignments_by_handle(handle_b).get("$b"), "value_b");
}

TEST_F(InMemoryDBTest, UpsertReplacesCustomAttributes) {
    auto a = new Node("Symbol", "\"a\"");
    auto b = new Node("Symbol", "\"b\"");
    auto similarity = new Node("Symbol", "Similarity");
    string a_h = db->add_node(a, false);
    string b_h = db->add_node(b, false);
    string sim_h = db->add_node(similarity, false);

    auto weak = new Link("Expression", {sim_h, a_h, b_h}, true, Properties{{"strength", 0.5}});
    string handle = db->add_link(weak, false);
    auto got_weak = db->get_atom(handle);
    ASSERT_NE(got_weak, nullptr);
    EXPECT_DOUBLE_EQ(got_weak->custom_attributes.get_or<double>("strength", -1.0), 0.5);

    auto strong = new Link("Expression", {sim_h, a_h, b_h}, true, Properties{{"strength", 0.9}});
    EXPECT_EQ(db->add_link(strong, false), handle);
    auto got_strong = db->get_atom(handle);
    ASSERT_NE(got_strong, nullptr);
    EXPECT_DOUBLE_EQ(got_strong->custom_attributes.get_or<double>("strength", -1.0), 0.9);

    // Incoming set must not grow from the upsert (same content-addressed handle/targets).
    auto incoming = db->query_for_incoming_set(a_h);
    ASSERT_NE(incoming, nullptr);
    EXPECT_EQ(incoming->size(), 1u);

    delete weak;
    delete strong;
    delete a;
    delete b;
    delete similarity;
}

TEST(HandleSetInMemoryTest, IteratorPointerOutlivesIterator) {
    auto set = make_shared<HandleSetInMemory>();
    const string handle = "0123456789abcdef0123456789abcdef";
    set->add_handle(handle);

    // Grab a pointer from the iterator, then destroy the iterator. The pointer must remain valid
    // because it points into storage owned by the HandleSet (mirrors HandleSetRedis semantics).
    char* captured = nullptr;
    {
        auto it = set->get_iterator();
        captured = it->next();
        ASSERT_NE(captured, nullptr);
        EXPECT_EQ(it->next(), nullptr);
    }  // iterator destroyed here

    ASSERT_NE(captured, nullptr);
    EXPECT_EQ(string(captured), handle);  // still valid while the set is alive
}

TEST(HandleSetInMemoryTest, IteratorVisitsAllHandlesOnce) {
    auto set = make_shared<HandleSetInMemory>();
    const string handle_a = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    const string handle_b = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    set->add_handle(handle_a);
    set->add_handle(handle_b);
    set->add_handle(handle_a);  // duplicate, set semantics collapse it

    EXPECT_EQ(set->size(), 2u);

    vector<string> visited;
    auto it = set->get_iterator();
    char* h;
    while ((h = it->next()) != nullptr) {
        visited.push_back(string(h));
    }
    EXPECT_EQ(visited.size(), 2u);
    EXPECT_NE(find(visited.begin(), visited.end(), handle_a), visited.end());
    EXPECT_NE(find(visited.begin(), visited.end(), handle_b), visited.end());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
