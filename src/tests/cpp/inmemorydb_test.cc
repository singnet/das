#include "InMemoryDB.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Assignment.h"
#include "InMemoryDBAPITypes.h"
#include "Link.h"
#include "LinkSchema.h"
#include "Merger.h"
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

    string human_handle = db->add_node(human);
    string monkey_handle = db->add_node(monkey);
    string chimp_handle = db->add_node(chimp);
    string mammal_handle = db->add_node(mammal);
    string similarity_handle = db->add_node(similarity);
    string inheritance_handle = db->add_node(inheritance);

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

    string link1_handle = db->add_link(link1);
    string link2_handle = db->add_link(link2);
    string link3_handle = db->add_link(link3);
    string link4_handle = db->add_link(link4);
    string link5_handle = db->add_link(link5);

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

    string human_handle = db->add_node(human);
    string monkey_handle = db->add_node(monkey);
    string chimp_handle = db->add_node(chimp);
    string mammal_handle = db->add_node(mammal);
    string inheritance_handle = db->add_node(inheritance);

    auto link1 = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});
    auto link2 = new Link("Expression", {inheritance_handle, monkey_handle, mammal_handle});
    auto link3 = new Link("Expression", {inheritance_handle, chimp_handle, mammal_handle});

    string link1_handle = db->add_link(link1);
    string link2_handle = db->add_link(link2);
    string link3_handle = db->add_link(link3);

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

    string human_handle = db->add_node(human);
    string monkey_handle = db->add_node(monkey);
    string similarity_handle = db->add_node(similarity);

    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});

    string link1_handle = db->add_link(link1);

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

    string node1_handle = db->add_node(node1);
    string node2_handle = db->add_node(node2);
    string node3_handle = db->add_node(node3);
    string similarity_handle = db->add_node(similarity);

    auto node_targets = db->query_for_targets(node1_handle);
    EXPECT_EQ(node_targets, nullptr);

    auto link1 = new Link("Expression", {similarity_handle, node1_handle, node2_handle, node3_handle});
    string link1_handle = db->add_link(link1);

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

    string human_handle = db->add_node(human);
    string monkey_handle = db->add_node(monkey);
    string chimp_handle = db->add_node(chimp);
    string similarity_handle = db->add_node(similarity);

    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    auto link2 = new Link("Expression", {similarity_handle, human_handle, chimp_handle});
    auto link3 = new Link("Expression", {similarity_handle, monkey_handle, chimp_handle});

    string link1_handle = db->add_link(link1);
    string link2_handle = db->add_link(link2);
    string link3_handle = db->add_link(link3);

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

    string node1_handle = db->add_node(node1);
    string node2_handle = db->add_node(node2);
    string similarity_handle = db->add_node(similarity);

    auto link1 = new Link("Expression", {similarity_handle, node1_handle, node2_handle});
    string link1_handle = db->add_link(link1);

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

    string human_handle = db->add_node(human);
    string monkey_handle = db->add_node(monkey);
    string chimp_handle = db->add_node(chimp);
    string mammal_handle = db->add_node(mammal);
    string similarity_handle = db->add_node(similarity);
    string inheritance_handle = db->add_node(inheritance);

    // Create links that reference human
    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    auto link2 = new Link("Expression", {similarity_handle, human_handle, chimp_handle});
    auto link3 = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});

    string link1_handle = db->add_link(link1);
    string link2_handle = db->add_link(link2);
    string link3_handle = db->add_link(link3);

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

    string human_handle = db->add_node(human);
    string monkey_handle = db->add_node(monkey);
    string similarity_handle = db->add_node(similarity);

    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link1_handle = db->add_link(link1);

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

    string human_handle = db->add_node(human);
    string monkey_handle = db->add_node(monkey);
    string similarity_handle = db->add_node(similarity);

    // Create a link that references human
    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link1_handle = db->add_link(link1);

    // Try to delete human atom with delete_link_targets=false (should fail)
    bool deleted = db->delete_atom(human_handle, false);
    EXPECT_FALSE(deleted);

    // Verify human still exists
    EXPECT_TRUE(db->node_exists(human_handle));
    EXPECT_TRUE(db->link_exists(link1_handle));

    // Create a link that references human
    auto link2 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link2_handle = db->add_link(link2);

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

    string human_handle = db->add_node(human);
    string monkey_handle = db->add_node(monkey);
    string similarity_handle = db->add_node(similarity);

    // Create a link that references human
    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link1_handle = db->add_link(link1);

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
    string link2_handle = db->add_link(link2);

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

    string human_handle = db->add_node(human);
    string monkey_handle = db->add_node(monkey);
    string similarity_handle = db->add_node(similarity);

    // Create a link that references human and monkey
    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link1_handle = db->add_link(link1);

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
    string link2_handle = db->add_link(link2);

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

    string human_handle = db->add_node(human);
    string monkey_handle = db->add_node(monkey);
    string chimp_handle = db->add_node(chimp);
    string similarity_handle = db->add_node(similarity);

    // Create two links that both reference human
    auto link1 = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    auto link2 = new Link("Expression", {similarity_handle, human_handle, chimp_handle});
    string link1_handle = db->add_link(link1);
    string link2_handle = db->add_link(link2);

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

    db->add_node(node1);
    db->add_node(node2);
    db->add_node(similarity);

    EXPECT_EQ(db->atom_count(), 3);

    auto link1 = new Link("Expression", {similarity->handle(), node1->handle(), node2->handle()});
    db->add_link(link1);

    EXPECT_EQ(db->atom_count(), 4);
    EXPECT_EQ(db->empty(), false);
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

namespace {

class SumStrengthMerger : public Merger {
   public:
    bool merge(Atom* existing, const Atom* incoming) const override {
        double existing_strength = existing->custom_attributes.get_or<double>("strength", 0.0);
        double incoming_strength = incoming->custom_attributes.get_or<double>("strength", 0.0);
        existing->custom_attributes["strength"] = existing_strength + incoming_strength;
        return true;
    }
};

}  // namespace

TEST_F(InMemoryDBTest, AddNodeReplacesByDefault) {
    Properties attrs1;
    attrs1["strength"] = 0.1;
    attrs1["obsolete"] = true;
    auto node1 = new Node("Symbol", "\"replace_me\"", attrs1);
    string handle = db->add_node(node1);
    EXPECT_DOUBLE_EQ(db->get_node(handle)->custom_attributes.get_or<double>("strength", -1.0), 0.1);
    EXPECT_TRUE(db->get_node(handle)->custom_attributes.get_or<bool>("obsolete", false));

    Properties attrs2;
    attrs2["strength"] = 0.9;
    auto node2 = new Node("Symbol", "\"replace_me\"", attrs2);
    EXPECT_EQ(db->add_node(node2), handle);
    EXPECT_DOUBLE_EQ(db->get_node(handle)->custom_attributes.get_or<double>("strength", -1.0), 0.9);
    EXPECT_FALSE(db->get_node(handle)->custom_attributes.get_or<bool>("obsolete", false));

    delete node1;
    delete node2;
}

TEST_F(InMemoryDBTest, AddNodeThrowIfExistsMerger) {
    auto node1 = new Node("Symbol", "\"throw_me\"");
    auto node2 = new Node("Symbol", "\"throw_me\"");
    EXPECT_EQ(db->add_node(node1, &ThrowIfExistsMerger::instance()), node1->handle());
    EXPECT_THROW(db->add_node(node2, &ThrowIfExistsMerger::instance()), runtime_error);
    delete node1;
    delete node2;
}

TEST_F(InMemoryDBTest, AddNodeCustomMerger) {
    Properties attrs1;
    attrs1["strength"] = 0.2;
    auto node1 = new Node("Symbol", "\"merge_me\"", attrs1);
    string handle = db->add_node(node1);

    Properties attrs2;
    attrs2["strength"] = 0.3;
    auto node2 = new Node("Symbol", "\"merge_me\"", attrs2);
    SumStrengthMerger merger;
    EXPECT_EQ(db->add_node(node2, &merger), handle);
    EXPECT_DOUBLE_EQ(db->get_node(handle)->custom_attributes.get_or<double>("strength", -1.0), 0.5);

    delete node1;
    delete node2;
}

TEST_F(InMemoryDBTest, AddLinkReplacesByDefault) {
    auto n1 = new Node("Symbol", "\"link_replace_a\"");
    auto n2 = new Node("Symbol", "\"link_replace_b\"");
    auto h1 = db->add_node(n1);
    auto h2 = db->add_node(n2);

    Properties attrs1;
    attrs1["strength"] = 0.1;
    attrs1["obsolete"] = true;
    auto link1 = new Link("Expression", {h1, h2}, attrs1);
    string handle = db->add_link(link1);
    EXPECT_DOUBLE_EQ(db->get_link(handle)->custom_attributes.get_or<double>("strength", -1.0), 0.1);
    EXPECT_TRUE(db->get_link(handle)->custom_attributes.get_or<bool>("obsolete", false));

    Properties attrs2;
    attrs2["strength"] = 0.9;
    auto link2 = new Link("Expression", {h1, h2}, attrs2);
    EXPECT_EQ(db->add_link(link2), handle);
    EXPECT_DOUBLE_EQ(db->get_link(handle)->custom_attributes.get_or<double>("strength", -1.0), 0.9);
    EXPECT_FALSE(db->get_link(handle)->custom_attributes.get_or<bool>("obsolete", false));

    auto incoming = db->query_for_incoming_set(h1);
    EXPECT_EQ(incoming->size(), 1u);

    delete n1;
    delete n2;
    delete link1;
    delete link2;
}

TEST_F(InMemoryDBTest, AddLinkThrowIfExistsMerger) {
    auto n1 = new Node("Symbol", "\"link_throw_a\"");
    auto n2 = new Node("Symbol", "\"link_throw_b\"");
    auto h1 = db->add_node(n1);
    auto h2 = db->add_node(n2);

    auto link1 = new Link("Expression", {h1, h2});
    auto link2 = new Link("Expression", {h1, h2});
    EXPECT_EQ(db->add_link(link1, &ThrowIfExistsMerger::instance()), link1->handle());
    EXPECT_THROW(db->add_link(link2, &ThrowIfExistsMerger::instance()), runtime_error);

    delete n1;
    delete n2;
    delete link1;
    delete link2;
}

TEST_F(InMemoryDBTest, AddLinkCustomMerger) {
    auto n1 = new Node("Symbol", "\"link_merge_a\"");
    auto n2 = new Node("Symbol", "\"link_merge_b\"");
    auto h1 = db->add_node(n1);
    auto h2 = db->add_node(n2);

    Properties attrs1;
    attrs1["strength"] = 0.2;
    auto link1 = new Link("Expression", {h1, h2}, attrs1);
    string handle = db->add_link(link1);

    Properties attrs2;
    attrs2["strength"] = 0.3;
    auto link2 = new Link("Expression", {h1, h2}, attrs2);
    SumStrengthMerger merger;
    EXPECT_EQ(db->add_link(link2, &merger), handle);
    EXPECT_DOUBLE_EQ(db->get_link(handle)->custom_attributes.get_or<double>("strength", -1.0), 0.5);

    delete n1;
    delete n2;
    delete link1;
    delete link2;
}

TEST_F(InMemoryDBTest, AddNodeSkipIfExistsMergerDoesNotPersist) {
    Properties attrs1;
    attrs1["strength"] = 0.2;
    auto node1 = new Node("Symbol", "\"reject_me\"", attrs1);
    string handle = db->add_node(node1);

    Properties attrs2;
    attrs2["strength"] = 0.9;
    auto node2 = new Node("Symbol", "\"reject_me\"", attrs2);
    EXPECT_EQ(db->add_node(node2, &SkipIfExistsMerger::instance()), "");
    EXPECT_DOUBLE_EQ(db->get_node(handle)->custom_attributes.get_or<double>("strength", -1.0), 0.2);

    delete node1;
    delete node2;
}

TEST_F(InMemoryDBTest, AddLinkSkipIfExistsMergerDoesNotPersistOrReindex) {
    auto n1 = new Node("Symbol", "\"link_reject_a\"");
    auto n2 = new Node("Symbol", "\"link_reject_b\"");
    auto h1 = db->add_node(n1);
    auto h2 = db->add_node(n2);

    Properties attrs1;
    attrs1["strength"] = 0.2;
    auto link1 = new Link("Expression", {h1, h2}, attrs1);
    string handle = db->add_link(link1);
    auto incoming_before = db->query_for_incoming_set(h1);
    ASSERT_EQ(incoming_before->size(), 1u);

    Properties attrs2;
    attrs2["strength"] = 0.9;
    auto link2 = new Link("Expression", {h1, h2}, attrs2);
    EXPECT_EQ(db->add_link(link2, &SkipIfExistsMerger::instance()), "");
    EXPECT_DOUBLE_EQ(db->get_link(handle)->custom_attributes.get_or<double>("strength", -1.0), 0.2);

    auto incoming_after = db->query_for_incoming_set(h1);
    EXPECT_EQ(incoming_after->size(), 1u);

    delete n1;
    delete n2;
    delete link1;
    delete link2;
}

TEST_F(InMemoryDBTest, AddNodesThrowIfExistsIsNotAllOrNothing) {
    auto existing = new Node("Symbol", "\"batch_throw_existing\"");
    db->add_node(existing);

    auto keep_absent = new Node("Symbol", "\"batch_throw_new_a\"");
    auto collision = new Node("Symbol", "\"batch_throw_existing\"");
    auto also_absent = new Node("Symbol", "\"batch_throw_new_b\"");

    // Earlier items may be applied; ThrowIfExists raises when the collision is hit.
    EXPECT_THROW(
        db->add_nodes({keep_absent, collision, also_absent}, false, &ThrowIfExistsMerger::instance()),
        runtime_error);
    EXPECT_TRUE(db->node_exists(keep_absent->handle()));
    EXPECT_FALSE(db->node_exists(also_absent->handle()));
    EXPECT_TRUE(db->node_exists(existing->handle()));

    // Duplicate only inside the batch: first insert succeeds, rematch raises.
    auto batch_a = new Node("Symbol", "\"batch_throw_dup_only\"");
    auto batch_a_copy = new Node("Symbol", "\"batch_throw_dup_only\"");
    auto batch_b = new Node("Symbol", "\"batch_throw_sibling\"");
    EXPECT_THROW(
        db->add_nodes({batch_a, batch_a_copy, batch_b}, false, &ThrowIfExistsMerger::instance()),
        runtime_error);
    EXPECT_TRUE(db->node_exists(batch_a->handle()));
    EXPECT_FALSE(db->node_exists(batch_b->handle()));

    delete existing;
    delete keep_absent;
    delete collision;
    delete also_absent;
    delete batch_a;
    delete batch_a_copy;
    delete batch_b;
}

TEST_F(InMemoryDBTest, AddLinksThrowIfExistsIsNotAllOrNothing) {
    auto n1 = new Node("Symbol", "\"batch_link_throw_a\"");
    auto n2 = new Node("Symbol", "\"batch_link_throw_b\"");
    auto n3 = new Node("Symbol", "\"batch_link_throw_c\"");
    auto n4 = new Node("Symbol", "\"batch_link_throw_d\"");
    auto n5 = new Node("Symbol", "\"batch_link_throw_e\"");
    auto n6 = new Node("Symbol", "\"batch_link_throw_f\"");
    auto h1 = db->add_node(n1);
    auto h2 = db->add_node(n2);
    auto h3 = db->add_node(n3);
    auto h4 = db->add_node(n4);
    auto h5 = db->add_node(n5);
    auto h6 = db->add_node(n6);

    auto existing = new Link("Expression", {h1, h2});
    db->add_link(existing);

    auto keep_absent = new Link("Expression", {h3, h4});
    auto collision = new Link("Expression", {h1, h2});

    EXPECT_THROW(db->add_links({keep_absent, collision}, false, &ThrowIfExistsMerger::instance()),
                 runtime_error);
    EXPECT_TRUE(db->link_exists(keep_absent->handle()));
    EXPECT_TRUE(db->link_exists(existing->handle()));
    EXPECT_EQ(db->query_for_incoming_set(h3)->size(), 1u);

    EXPECT_TRUE(db->delete_link(keep_absent->handle()));

    auto batch_dup = new Link("Expression", {h5, h6});
    auto batch_dup_copy = new Link("Expression", {h5, h6});
    auto batch_other = new Link("Expression", {h3, h4});
    EXPECT_THROW(
        db->add_links({batch_dup, batch_dup_copy, batch_other}, false, &ThrowIfExistsMerger::instance()),
        runtime_error);
    EXPECT_TRUE(db->link_exists(batch_dup->handle()));
    EXPECT_FALSE(db->link_exists(batch_other->handle()));
    EXPECT_EQ(db->query_for_incoming_set(h5)->size(), 1u);

    delete n1;
    delete n2;
    delete n3;
    delete n4;
    delete n5;
    delete n6;
    delete existing;
    delete keep_absent;
    delete collision;
    delete batch_dup;
    delete batch_dup_copy;
    delete batch_other;
}

TEST_F(InMemoryDBTest, AddNodesSkipIfExistsMergerReturnsEmptyHandleSlots) {
    Properties attrs1;
    attrs1["strength"] = 0.2;
    auto existing = new Node("Symbol", "\"batch_reject_existing\"", attrs1);
    string existing_handle = db->add_node(existing);

    auto fresh = new Node("Symbol", "\"batch_reject_new\"");
    Properties attrs2;
    attrs2["strength"] = 0.9;
    auto collision = new Node("Symbol", "\"batch_reject_existing\"", attrs2);

    auto handles = db->add_nodes({fresh, collision}, false, &SkipIfExistsMerger::instance());
    ASSERT_EQ(handles.size(), 2u);
    EXPECT_EQ(handles[0], fresh->handle());
    EXPECT_EQ(handles[1], "");
    EXPECT_TRUE(db->node_exists(fresh->handle()));
    EXPECT_DOUBLE_EQ(db->get_node(existing_handle)->custom_attributes.get_or<double>("strength", -1.0),
                     0.2);

    delete existing;
    delete fresh;
    delete collision;
}

TEST_F(InMemoryDBTest, AddLinksSkipIfExistsMergerReturnsEmptyHandleSlots) {
    auto n1 = new Node("Symbol", "\"batch_link_reject_a\"");
    auto n2 = new Node("Symbol", "\"batch_link_reject_b\"");
    auto n3 = new Node("Symbol", "\"batch_link_reject_c\"");
    auto n4 = new Node("Symbol", "\"batch_link_reject_d\"");
    auto h1 = db->add_node(n1);
    auto h2 = db->add_node(n2);
    auto h3 = db->add_node(n3);
    auto h4 = db->add_node(n4);

    Properties attrs1;
    attrs1["strength"] = 0.2;
    auto existing = new Link("Expression", {h1, h2}, attrs1);
    string existing_handle = db->add_link(existing);

    auto fresh = new Link("Expression", {h3, h4});
    Properties attrs2;
    attrs2["strength"] = 0.9;
    auto collision = new Link("Expression", {h1, h2}, attrs2);

    auto handles = db->add_links({fresh, collision}, false, &SkipIfExistsMerger::instance());
    ASSERT_EQ(handles.size(), 2u);
    EXPECT_EQ(handles[0], fresh->handle());
    EXPECT_EQ(handles[1], "");
    EXPECT_TRUE(db->link_exists(fresh->handle()));
    EXPECT_DOUBLE_EQ(db->get_link(existing_handle)->custom_attributes.get_or<double>("strength", -1.0),
                     0.2);
    EXPECT_EQ(db->query_for_incoming_set(h1)->size(), 1u);

    delete n1;
    delete n2;
    delete n3;
    delete n4;
    delete existing;
    delete fresh;
    delete collision;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
