#include "MorkDB.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "AtomDBCacheSingleton.h"
#include "AtomDBSingleton.h"
#include "LinkSchema.h"
#include "MockAnimalsData.h"
#include "TestConfig.h"

using namespace atomdb;
using namespace atoms;
using namespace std;

class MorkDBTestEnvironment : public ::testing::Environment {
   public:
    void SetUp() override {
        TestConfig::load_environment();
        TestConfig::set_atomdb_cache(false);
        auto atomdb = new MorkDB("morkdb_test_");
        atomdb->drop_all();
        AtomDBSingleton::provide(shared_ptr<AtomDB>(atomdb));
        load_animals_data();
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
            variable, "x",
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
    auto node1 = new Node("Symbol", "QueryForTargetsNode1");
    auto node1_handle = db->add_node(node1);
    auto node2 = new Node("Symbol", "QueryForTargetsNode2");
    auto node2_handle = db->add_node(node2);
    auto node3 = new Node("Symbol", "QueryForTargetsNode3");
    auto node3_handle = db->add_node(node3);

    auto node1_targets = db->query_for_targets(node1_handle);
    EXPECT_EQ(node1_targets, nullptr);

    auto link1 = new Link("Expression", {node1_handle, node2_handle, node3_handle});
    auto link1_handle = db->add_link(link1);

    ASSERT_TRUE(db->link_exists(link1_handle));

    auto link1_targets = db->query_for_targets(link1_handle);
    EXPECT_NE(link1_targets, nullptr);
    EXPECT_EQ(link1_targets->size(), 3);
    EXPECT_EQ(link1_targets->get_handle(0), node1_handle);
    EXPECT_EQ(link1_targets->get_handle(1), node2_handle);
    EXPECT_EQ(link1_targets->get_handle(2), node3_handle);
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
                    variable, "x",
                    variable, "y"});
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
            variable, "x",
            variable, "y"});
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

TEST_F(MorkDBTest, AddLinksWithDuplicateTargets) {
    vector<Node*> nodes;
    nodes.push_back(new Node("Symbol", "DuplicateTargets1"));
    nodes.push_back(new Node("Symbol", "DuplicateTargets2"));
    nodes.push_back(new Node("Symbol", "DuplicateTargets3"));
    EXPECT_EQ(db->add_nodes(nodes, true).size(), 3);

    auto link = new Link("Expression",
                         {nodes[0]->handle(),
                          nodes[1]->handle(),
                          nodes[1]->handle(),
                          nodes[0]->handle(),
                          nodes[2]->handle(),
                          nodes[0]->handle(),
                          nodes[2]->handle()});
    EXPECT_EQ(db->add_link(link), link->handle());
    // MORKDB does not support deleting links, so it must always return false
    EXPECT_FALSE(db->delete_link(link->handle(), true));
}

TEST_F(MorkDBTest, ConcurrentAddLinks) {
    int num_links = 5000;
    int arity = 3;
    int chunck_size = 500;

    const int num_threads = 100;
    vector<thread> threads;
    atomic<int> success_count{0};

    auto worker = [&](int thread_id) {
        try {
            vector<Node*> nodes;
            vector<Link*> links;
            for (int i = 1; i <= num_links; i++) {
                string metta_expression = "(";
                vector<string> targets;
                vector<string> node_names;
                for (int j = 1; j <= arity; j++) {
                    string name = "ConcurrentAddLinks-" + to_string(thread_id) + "-" + to_string(i) +
                                  "-" + to_string(j);
                    auto node = new Node("Symbol", name);
                    targets.push_back(node->handle());
                    nodes.push_back(node);
                    node_names.push_back(name);
                    metta_expression += name;
                    if (j != arity) metta_expression += " ";
                }
                metta_expression += ")";

                auto link = new Link(
                    "Expression", targets, true, Properties{{"metta_expression", metta_expression}});
                links.push_back(link);

                vector<string> nested_targets = {targets[0], targets[1], link->handle()};
                string nested_metta_expression =
                    "(" + node_names[0] + " " + node_names[1] + " " + metta_expression + ")";
                auto link_with_nested =
                    new Link("Expression",
                             nested_targets,
                             true,
                             Properties{{"metta_expression", nested_metta_expression}});
                links.push_back(link_with_nested);

                if (i % chunck_size == 0) {
                    db->add_nodes(nodes, false, true);
                    db->add_links(links, false, true);
                    nodes.clear();
                    links.clear();
                }
            }

            if (!nodes.empty()) db->add_nodes(nodes, false, true);
            if (!links.empty()) db->add_links(links, false, true);

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

    // clang-format off
    LinkSchema link_schema({
        "LINK_TEMPLATE", "Expression", "3", 
        "NODE", "Symbol", "ConcurrentAddLinks-0-1-1", 
        "NODE", "Symbol", "ConcurrentAddLinks-0-1-2",
        "VARIABLE", "V"
    });
    // clang-format on

    auto result = db->query_for_pattern(link_schema);
    EXPECT_EQ(result->size(), 2);
}

TEST_F(MorkDBTest, AddLinkWithoutMettaExpressionMustPopulateIt) {
    auto similarity = new Node("Symbol", "Similarity");
    auto human = new Node("Symbol", "\"human\"");
    auto robot = new Node("Symbol", "\"robot\"");

    db->add_node(similarity, false);
    db->add_node(human, false);
    auto robot_handle = db->add_node(robot);

    auto link = new Link("Expression", {similarity->handle(), human->handle(), robot->handle()});
    EXPECT_EQ(link->custom_attributes.get_or<string>("metta_expression", ""), "");

    auto link_handle = db->add_link(link);

    auto link_document =
        dynamic_pointer_cast<atomdb_api_types::MongodbDocument>(db->get_link_document(link_handle));

    Properties custom_attributes;
    if (link_document->contains("custom_attributes")) {
        custom_attributes =
            link_document->extract_custom_attributes(link_document->get_object("custom_attributes"));
    }

    EXPECT_EQ(custom_attributes.get_or<string>("metta_expression", ""),
              string("(Similarity \"human\" \"robot\")"));
}

TEST_F(MorkDBTest, ReIndexPatterns) {
    auto evaluation_node = new Node("Symbol", "EvaluationReIndex");
    vector<string> targets1 = {
        evaluation_node->handle(), exp_hash({"Predicate", "link"}), exp_hash({"Concept", "One"})};
    vector<string> targets2 = {
        evaluation_node->handle(), exp_hash({"Predicate", "link"}), exp_hash({"Concept", "Two"})};
    vector<string> targets3 = {
        evaluation_node->handle(), exp_hash({"Predicate", "link"}), exp_hash({"Concept", "Three"})};

    auto p1 = Properties();
    p1["metta_expression"] = "(EvaluationReIndex (Predicate \"link\") (Concept \"One\"))";
    auto link_1 = new Link("Expression", targets1, true, p1);

    auto p2 = Properties();
    p2["metta_expression"] = "(EvaluationReIndex (Predicate \"link\") (Concept \"Two\"))";
    auto link_2 = new Link("Expression", targets2, true, p2);

    auto p3 = Properties();
    p3["metta_expression"] = "(EvaluationReIndex (Predicate \"link\") (Concept \"Three\"))";
    auto link_3 = new Link("Expression", targets3, true, p3);

    auto link_1_doc = make_shared<atomdb_api_types::MongodbDocument>(link_1, "", vector<string>{});
    auto link_2_doc = make_shared<atomdb_api_types::MongodbDocument>(link_2, "", vector<string>{});
    auto link_3_doc = make_shared<atomdb_api_types::MongodbDocument>(link_3, "", vector<string>{});

    // clang-format off
    LinkSchema link_schema({
        "LINK_TEMPLATE", "Expression", "3", 
        "NODE", "Symbol", "EvaluationReIndex", 
        "LINK", "Expression", "2", "NODE", "Symbol", "Predicate", "NODE", "Symbol", "\"link\"",
        "VARIABLE", "C"
    });
    // clang-format on
    auto result = db->query_for_pattern(link_schema);
    EXPECT_EQ(result->size(), 0);

    // Resetting MongoDB database to animals data
    load_animals_data();

    // Direct insertion into MongoDB
    auto inserted_count =
        db->upsert_documents({link_1_doc->value(), link_2_doc->value(), link_3_doc->value()},
                             RedisMongoDB::MONGODB_LINKS_COLLECTION_NAME);
    EXPECT_EQ(inserted_count, 3);

    db->re_index_patterns();

    result = db->query_for_pattern(link_schema);
    EXPECT_EQ(result->size(), 3);

    string pattern = "(EvaluationReIndex $P $C)";
    db->flush_pattern(pattern);

    result = db->query_for_pattern(link_schema);
    EXPECT_EQ(result->size(), 0);

    // Confirm that patterns are re-indexed

    // clang-format off
    link_schema = LinkSchema({
        link_template, expression, "3",
            node, symbol, inheritance,
            variable, "x",
            node, symbol, mammal});
    // clang-format on

    result = db->query_for_pattern(link_schema);
    EXPECT_EQ(result->size(), 4);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new MorkDBTestEnvironment());
    return RUN_ALL_TESTS();
}
