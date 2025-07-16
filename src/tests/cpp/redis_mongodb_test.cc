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
#include "TestConfig.h"

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
        TestConfig::load_environment();
        TestConfig::disable_atomdb_cache();
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

TEST_F(RedisMongoDBTest, ConcurrentQueryForPattern) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    auto worker = [&](int thread_id) {
        try {
            auto link_schema = new LinkSchemaHandle("e8ca47108af6d35664f8813e1f96c5fa");
            auto handle_set = db->query_for_pattern(*link_schema);
            ASSERT_NE(handle_set, nullptr);
            ASSERT_EQ(handle_set->size(), 3);
            success_count++;
            delete link_schema;
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

    // Test non-existing pattern
    auto link_schema = new LinkSchemaHandle("00000000000000000000000000000000");
    auto handle_set = db->query_for_pattern(*link_schema);
    delete link_schema;
    EXPECT_EQ(handle_set->size(), 0);
}

TEST_F(RedisMongoDBTest, ConcurrentQueryForTargets) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    auto worker = [&](int thread_id) {
        try {
            auto targets = db->query_for_targets("68ea071c32d4dbf0a7d8e8e00f2fb823");
            ASSERT_NE(targets, nullptr);
            ASSERT_EQ(targets->size(), 3);
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

    // Test non-existing link
    auto targets = db->query_for_targets("00000000000000000000000000000000");
    EXPECT_EQ(targets, nullptr);
}

TEST_F(RedisMongoDBTest, ConcurrentGetAtomDocument) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    string human_handle = terminal_hash((char*) "Symbol", (char*) "\"human\"");
    string monkey_handle = terminal_hash((char*) "Symbol", (char*) "\"monkey\"");
    string chimp_handle = terminal_hash((char*) "Symbol", (char*) "\"chimp\"");
    string ent_handle = terminal_hash((char*) "Symbol", (char*) "\"ent\"");

    vector<string> doc_handles;
    while (doc_handles.size() < num_threads) {
        doc_handles.push_back(human_handle);
        doc_handles.push_back(monkey_handle);
        doc_handles.push_back(chimp_handle);
        doc_handles.push_back(ent_handle);
    }

    auto worker = [&](int thread_id) {
        try {
            auto doc = db->get_atom_document(doc_handles[thread_id]);
            if (doc != nullptr) {
                success_count++;
            }
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

    // Test non-existing handle
    string non_existing_handle = terminal_hash((char*) "Symbol", (char*) "\"non-existing\"");
    auto doc = db->get_atom_document(non_existing_handle);
    EXPECT_EQ(doc, nullptr);
}

TEST_F(RedisMongoDBTest, ConcurrentGetAtomDocuments) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    string handle_1 = exp_hash({"Similarity", "\"human\"", "\"monkey\""});
    string handle_2 = exp_hash({"Similarity", "\"human\"", "\"chimp\""});
    string handle_3 = exp_hash({"Similarity", "\"human\"", "\"ent\""});
    string handle_4 = exp_hash({"Similarity", "\"chimp\"", "\"monkey\""});

    auto worker = [&](int thread_id) {
        try {
            auto docs = db->get_atom_documents({handle_1, handle_2, handle_3, handle_4}, {"targets"});
            if (docs.size() == 4) {
                success_count++;
            }
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

    // Test non-existing handle
    string non_existing_handle = exp_hash({"Similarity", "\"non\"", "\"existing\""});
    auto docs = db->get_atom_documents({non_existing_handle}, {"targets"});
    EXPECT_EQ(docs.size(), 0);
}

TEST_F(RedisMongoDBTest, ConcurrentLinkExists) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    auto worker = [&](int thread_id) {
        try {
            auto link_exists = db->link_exists("68ea071c32d4dbf0a7d8e8e00f2fb823");
            ASSERT_TRUE(link_exists);
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

    // Test non-existing link
    auto link_exists = db->link_exists("00000000000000000000000000000000");
    EXPECT_FALSE(link_exists);
}

TEST_F(RedisMongoDBTest, ConcurrentLinksExist) {
    const int num_threads = 200;
    vector<thread> threads;
    atomic<int> success_count{0};

    auto worker = [&](int thread_id) {
        try {
            auto links_exist = db->links_exist({"68ea071c32d4dbf0a7d8e8e00f2fb823",
                                                "00000000000000000000000000000000",
                                                "7ec8526b8c8f15a6ac55273fedbf694f"});
            ASSERT_EQ(links_exist.size(), 2);
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

    // Test non-existing link
    auto links_exist = db->links_exist({"00000000000000000000000000000000",
                                        "00000000000000000000000000000001",
                                        "00000000000000000000000000000002"});
    EXPECT_EQ(links_exist.size(), 0);
}

TEST_F(RedisMongoDBTest, ConcurrentAddNodesAndLinks) {
    const int num_threads = 100;
    vector<thread> threads;
    atomic<int> success_count{0};

    auto worker = [&](int thread_id) {
        try {
            auto n1 = new Node("Symbol", "n1-" + to_string(thread_id));
            auto n2 = new Node("Symbol", "n2-" + to_string(thread_id));
            auto link_node = new Node("Symbol", "link-" + to_string(thread_id));
            auto link = new Link("Expression", {link_node->handle(), n1->handle(), n2->handle()});

            db->add_node(n1);
            db->add_node(n2);
            db->add_node(link_node);

            db->add_link(link);

            ASSERT_TRUE(db->node_exists(n1->handle()));
            ASSERT_TRUE(db->node_exists(n2->handle()));
            ASSERT_TRUE(db->node_exists(link_node->handle()));
            ASSERT_TRUE(db->link_exists(link->handle()));

            EXPECT_TRUE(db->delete_atom(link->handle()));
            EXPECT_TRUE(db->delete_atom(link_node->handle()));
            EXPECT_TRUE(db->delete_atom(n1->handle()));
            EXPECT_TRUE(db->delete_atom(n2->handle()));

            ASSERT_FALSE(db->link_exists(link->handle()));
            ASSERT_FALSE(db->link_exists(link_node->handle()));
            ASSERT_FALSE(db->node_exists(n1->handle()));
            ASSERT_FALSE(db->node_exists(n2->handle()));

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
}

TEST_F(RedisMongoDBTest, AddAndDeleteNode) {
    Properties custom_attributes;
    custom_attributes["is_literal"] = true;

    auto node = new Node("Symbol", "\"test-1\"", custom_attributes);

    auto node_handle = node->handle();

    // Check if node exists, if so, delete it
    auto node_document = db->get_atom_document(node_handle);
    if (node_document != nullptr) {
        auto deleted = db->delete_atom(node_handle.c_str());
        EXPECT_TRUE(deleted);
    }

    auto handle = db->add_node(node);
    EXPECT_NE(handle, "");

    auto deleted = db->delete_atom(handle);
    EXPECT_TRUE(deleted);
}

TEST_F(RedisMongoDBTest, AddAndDeleteNodes) {
    vector<Node*> nodes;
    for (int i = 0; i < 10; i++) {
        nodes.push_back(new Node("Symbol", "add-nodes-" + to_string(i)));
    }

    auto handles = db->add_nodes(nodes);
    EXPECT_EQ(handles.size(), 10);

    auto nodes_documents = db->get_atom_documents(handles, {"_id"});
    EXPECT_EQ(nodes_documents.size(), 10);

    auto deleted = db->delete_atoms(handles);
    EXPECT_EQ(deleted, 10);

    auto nodes_documents_after_delete = db->get_atom_documents(handles, {"_id"});
    EXPECT_EQ(nodes_documents_after_delete.size(), 0);
}

TEST_F(RedisMongoDBTest, AddAndDeleteLink) {
    MockDecoder decoder;
    Properties custom_attributes;
    custom_attributes["is_toplevel"] = true;

    string symbol = MettaMapping::SYMBOL_NODE_TYPE;
    string expression = MettaMapping::EXPRESSION_LINK_TYPE;
    vector<string> v;

    auto similarity_node = decoder.add_atom(make_shared<Node>(symbol, "Similarity"));
    auto test_1_node = decoder.add_atom(make_shared<Node>(symbol, "\"test-1\""));
    auto test_2_node = decoder.add_atom(make_shared<Node>(symbol, "\"test-2\""));

    auto test_1_node_handle = db->add_node((Node*) test_1_node.get());
    auto test_2_node_handle = db->add_node((Node*) test_2_node.get());

    auto link = new Link("Expression",
                         {similarity_node->handle(), test_1_node->handle(), test_2_node->handle()},
                         custom_attributes);

    auto link_handle = link->handle();

    // Check if link exists, if so, delete it
    auto link_exists = db->link_exists(link_handle.c_str());
    if (link_exists) {
        auto deleted = db->delete_atom(link_handle.c_str());
        EXPECT_TRUE(deleted);
    }

    auto handle = db->add_link(link);
    EXPECT_NE(handle, "");

    EXPECT_TRUE(db->delete_atom(handle));
    EXPECT_TRUE(db->delete_atom(test_1_node_handle));
    EXPECT_TRUE(db->delete_atom(test_2_node_handle));
}

TEST_F(RedisMongoDBTest, AddAndDeleteLinks) {
    vector<Link*> links;
    vector<string> test_node_handles;
    MockDecoder decoder;

    auto similarity_node = new Node("Symbol", "Similarity");
    for (int i = 0; i < 10; i++) {
        auto test_1_node = decoder.add_atom(make_shared<Node>("Symbol", "add-links-1-" + to_string(i)));
        auto test_2_node = decoder.add_atom(make_shared<Node>("Symbol", "add-links-2-" + to_string(i)));
        test_node_handles.push_back(db->add_node((Node*) test_1_node.get()));
        test_node_handles.push_back(db->add_node((Node*) test_2_node.get()));
        links.push_back(new Link(
            "Expression", {similarity_node->handle(), test_1_node->handle(), test_2_node->handle()}));
    }

    auto handles = db->add_links(links);
    EXPECT_EQ(handles.size(), 10);

    auto links_exist = db->links_exist(handles);
    EXPECT_EQ(links_exist.size(), 10);

    EXPECT_EQ(db->delete_atoms(handles), 10);

    auto links_exist_after_delete = db->links_exist(handles);
    EXPECT_EQ(links_exist_after_delete.size(), 0);

    EXPECT_EQ(db->delete_nodes(test_node_handles), test_node_handles.size());
}

TEST_F(RedisMongoDBTest, DeleteNodesAndLinks) {
    vector<atoms::Node*> nodes;
    vector<atoms::Link*> links;
    MockDecoder decoder;

    auto similarity_node = new Node("Symbol", "Similarity");
    for (int i = 0; i < 5555; i++) {
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

TEST_F(RedisMongoDBTest, DeleteLinkAndDeleteItsTargets) {
    vector<string> nodes_handles;

    auto link_name_node = new Node("Symbol", "TestLinkName");
    nodes_handles.push_back(db->add_node(link_name_node));

    auto test_1_node = new Node("Symbol", "del-links-1");
    nodes_handles.push_back(db->add_node(test_1_node));
    auto test_2_node = new Node("Symbol", "del-links-2");
    nodes_handles.push_back(db->add_node(test_2_node));

    auto link =
        new Link("Expression", {link_name_node->handle(), test_1_node->handle(), test_2_node->handle()});
    auto link_handle = db->add_link(link);

    EXPECT_TRUE(db->delete_link(link_handle));
    EXPECT_FALSE(db->link_exists(link_handle));

    EXPECT_EQ(db->delete_nodes(nodes_handles), 3);
    EXPECT_EQ(db->nodes_exist(nodes_handles).size(), 0);
}

TEST_F(RedisMongoDBTest, DeleteLinkWithNestedLink) {
    // Delete a link with a nested links
    // (TestLinkName (TestNestedLinkName1 (TestNestedLinkName2 N1 N2) N3) N4)
    vector<string> handles;

    auto link_node = new Node("Symbol", "TestLink");
    handles.push_back(db->add_node(link_node));

    auto nested_link_node_1 = new Node("Symbol", "TestNestedLink1");
    handles.push_back(db->add_node(nested_link_node_1));
    auto nested_link_node_2 = new Node("Symbol", "TestNestedLink2");
    handles.push_back(db->add_node(nested_link_node_2));

    auto n1_node = new Node("Symbol", "N1");
    handles.push_back(db->add_node(n1_node));
    auto n2_node = new Node("Symbol", "N2");
    handles.push_back(db->add_node(n2_node));
    auto n3_node = new Node("Symbol", "N3");
    handles.push_back(db->add_node(n3_node));
    auto n4_node = new Node("Symbol", "N4");
    handles.push_back(db->add_node(n4_node));

    auto nested_link_2 =
        new Link("Expression", {nested_link_node_2->handle(), n1_node->handle(), n2_node->handle()});
    handles.push_back(db->add_link(nested_link_2));

    auto nested_link_1 = new Link(
        "Expression", {nested_link_node_1->handle(), nested_link_2->handle(), n3_node->handle()});
    handles.push_back(db->add_link(nested_link_1));

    auto link =
        new Link("Expression", {link_node->handle(), nested_link_1->handle(), n4_node->handle()});
    handles.push_back(db->add_link(link));

    // Delete nested_link_1 means deleting link but not nested_link_2.
    EXPECT_TRUE(db->delete_link(nested_link_1->handle()));
    EXPECT_FALSE(db->link_exists(link->handle()));
    EXPECT_FALSE(db->link_exists(nested_link_1->handle()));
    EXPECT_TRUE(db->link_exists(nested_link_2->handle()));

    // Before delete: 7 nodes + 3 links
    // After delete: 7 nodes + 1 link (nested_link_2)
    EXPECT_EQ(db->atoms_exist(handles).size(), 8);

    db->delete_atoms(handles);
    EXPECT_EQ(db->atoms_exist(handles).size(), 0);
}

TEST_F(RedisMongoDBTest, DeleteLinkWithNestedLinkAndDeleteTargets) {
    // Delete a link with a nested links
    // (TestLinkName (TestNestedLinkName1 (TestNestedLinkName2 N1 N2) N3) N4)
    vector<string> handles;

    auto link_node = new Node("Symbol", "TestLink");
    handles.push_back(db->add_node(link_node));

    auto nested_link_node_1 = new Node("Symbol", "TestNestedLink1");
    handles.push_back(db->add_node(nested_link_node_1));
    auto nested_link_node_2 = new Node("Symbol", "TestNestedLink2");
    handles.push_back(db->add_node(nested_link_node_2));

    auto n1_node = new Node("Symbol", "N1");
    handles.push_back(db->add_node(n1_node));
    auto n2_node = new Node("Symbol", "N2");
    handles.push_back(db->add_node(n2_node));
    auto n3_node = new Node("Symbol", "N3");
    handles.push_back(db->add_node(n3_node));
    auto n4_node = new Node("Symbol", "N4");
    handles.push_back(db->add_node(n4_node));

    auto nested_link_2 =
        new Link("Expression", {nested_link_node_2->handle(), n1_node->handle(), n2_node->handle()});
    handles.push_back(db->add_link(nested_link_2));

    auto nested_link_1 = new Link(
        "Expression", {nested_link_node_1->handle(), nested_link_2->handle(), n3_node->handle()});
    handles.push_back(db->add_link(nested_link_1));

    auto link =
        new Link("Expression", {link_node->handle(), nested_link_1->handle(), n4_node->handle()});
    handles.push_back(db->add_link(link));

    // Delete nested_link_1 and its targets means deleting link and nested_link_2 (recursively).
    EXPECT_TRUE(db->delete_link(nested_link_1->handle(), true));
    EXPECT_EQ(db->atoms_exist(handles).size(), 0);
}

TEST_F(RedisMongoDBTest, DeleteLinkWithTargetsUsedByOtherLinks) {
    vector<string> handles;

    // This node is referenced by other links.
    auto similarity_node = new Node("Symbol", "Similarity");
    auto handle_set = db->query_for_incoming_set(similarity_node->handle());
    EXPECT_EQ(handle_set->size(), 15);

    auto test_1_node = new Node("Symbol", "Test1");
    auto test_2_node = new Node("Symbol", "Test2");
    handles.push_back(db->add_node(test_1_node));
    handles.push_back(db->add_node(test_2_node));

    auto link = new Link("Expression",
                         {similarity_node->handle(), test_1_node->handle(), test_2_node->handle()});
    handles.push_back(db->add_link(link));

    handle_set = db->query_for_incoming_set(similarity_node->handle());
    EXPECT_EQ(handle_set->size(), 16);

    EXPECT_TRUE(db->delete_link(link->handle(), true));
    handle_set = db->query_for_incoming_set(similarity_node->handle());
    EXPECT_EQ(handle_set->size(), 15);

    EXPECT_EQ(db->atoms_exist(handles).size(), 0);
}

TEST_F(RedisMongoDBTest, QueryForIncomingSet) {
    vector<string> handles;

    auto symbol = new Node("Symbol", "S");
    auto n1 = new Node("Symbol", "N1");
    auto n2 = new Node("Symbol", "N2");

    handles.push_back(db->add_node(symbol));
    handles.push_back(db->add_node(n1));
    handles.push_back(db->add_node(n2));

    auto link_1 = new Link("Expression", {symbol->handle(), n1->handle(), n2->handle()});
    auto link_2 = new Link("Expression", {symbol->handle(), n2->handle(), n1->handle()});
    handles.push_back(db->add_link(link_1));
    handles.push_back(db->add_link(link_2));

    auto handle_set = db->query_for_incoming_set(symbol->handle());
    EXPECT_EQ(handle_set->size(), 2);

    EXPECT_TRUE(db->delete_link(link_1->handle()));
    handle_set = db->query_for_incoming_set(symbol->handle());
    EXPECT_EQ(handle_set->size(), 1);

    EXPECT_TRUE(db->delete_link(link_2->handle(), true));
    handle_set = db->query_for_incoming_set(symbol->handle());
    EXPECT_EQ(handle_set->size(), 0);

    EXPECT_EQ(db->atoms_exist(handles).size(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new RedisMongoDBTestEnvironment());
    return RUN_ALL_TESTS();
}
