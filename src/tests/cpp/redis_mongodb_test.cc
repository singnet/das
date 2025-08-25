#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "Atom.h"
#include "AtomDBSingleton.h"
#include "Hasher.h"
#include "Link.h"
#include "MettaMapping.h"
#include "MockAnimalsData.h"
#include "Node.h"
#include "RedisMongoDB.h"
#include "TestConfig.h"
#include "UntypedVariable.h"
#include "Wildcard.h"

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
        auto atomdb = new RedisMongoDB("test_");
        AtomDBSingleton::provide(shared_ptr<AtomDB>(atomdb));
        load_animals_data();
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

    string symbol = MettaMapping::SYMBOL_NODE_TYPE;
    string expression = MettaMapping::EXPRESSION_LINK_TYPE;
    vector<string> v;

    auto similarity_node = decoder.add_atom(make_shared<Node>(symbol, "Similarity"));
    auto test_1_node = decoder.add_atom(make_shared<Node>(symbol, "\"test-1\""));
    auto test_2_node = decoder.add_atom(make_shared<Node>(symbol, "\"test-2\""));

    auto test_1_node_handle = db->add_node((Node*) test_1_node.get());
    auto test_2_node_handle = db->add_node((Node*) test_2_node.get());

    bool is_toplevel = true;

    auto link = new Link("Expression",
                         {similarity_node->handle(), test_1_node->handle(), test_2_node->handle()},
                         is_toplevel);

    auto link_handle = link->handle();

    // Check if link exists, if so, delete it
    auto link_exists = db->link_exists(link_handle.c_str());
    if (link_exists) {
        auto deleted = db->delete_atom(link_handle.c_str());
        EXPECT_TRUE(deleted);
    }

    auto handle = db->add_link(link);
    EXPECT_NE(handle, "");

    auto link_document = db->get_atom_document(handle);

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

TEST_F(RedisMongoDBTest, QueryForSimilarityIncomingSet) {
    vector<string> handles;

    // This node is referenced by other links.
    auto similarity = new Node("Symbol", "Similarity");
    auto handle_set = db->query_for_incoming_set(similarity->handle());
    EXPECT_EQ(handle_set->size(), 15);

    auto n1 = new Node("Symbol", "N1");
    auto n2 = new Node("Symbol", "N2");
    handles.push_back(db->add_node(n1));
    handles.push_back(db->add_node(n2));

    auto link = new Link("Expression", {similarity->handle(), n1->handle(), n2->handle()});
    handles.push_back(db->add_link(link));

    handle_set = db->query_for_incoming_set(similarity->handle());
    EXPECT_EQ(handle_set->size(), 16);

    EXPECT_TRUE(db->delete_link(link->handle(), true));
    handle_set = db->query_for_incoming_set(similarity->handle());
    EXPECT_EQ(handle_set->size(), 15);

    EXPECT_EQ(db->atoms_exist(handles).size(), 0);
}

TEST_F(RedisMongoDBTest, AddLinkAndQueryForSimilarityPattern) {
    vector<string> handles;

    // (Similarity * *)
    auto similarity_link_schema = new LinkSchemaHandle("dabd1f087cf4a9739911c0385fae0819");
    auto handle_set = db->query_for_pattern(*similarity_link_schema);
    EXPECT_EQ(handle_set->size(), 14);

    auto symbol = new Node("Symbol", "Similarity");
    auto n1 = new Node("Symbol", "N1");
    auto n2 = new Node("Symbol", "N2");

    handles.push_back(db->add_node(n1));
    handles.push_back(db->add_node(n2));

    auto link_1 = new Link("Expression", {symbol->handle(), n1->handle(), n2->handle()});
    auto link_2 = new Link("Expression", {symbol->handle(), n2->handle(), n1->handle()});
    handles.push_back(db->add_link(link_1));
    handles.push_back(db->add_link(link_2));

    // (Similarity * *) must have 2 more links
    handle_set = db->query_for_pattern(*similarity_link_schema);
    EXPECT_EQ(handle_set->size(), 16);

    EXPECT_TRUE(db->delete_link(link_1->handle()));
    handle_set = db->query_for_pattern(*similarity_link_schema);
    EXPECT_EQ(handle_set->size(), 15);

    EXPECT_TRUE(db->delete_link(link_2->handle(), true));
    handle_set = db->query_for_pattern(*similarity_link_schema);
    EXPECT_EQ(handle_set->size(), 14);

    EXPECT_EQ(db->atoms_exist(handles).size(), 0);
}

TEST_F(RedisMongoDBTest, GetAtomWithCustomAttributes) {
    auto node_with_no_custom_attributes = new Node("Symbol", "NodeWithNoCustomAttributes");
    db->add_node(node_with_no_custom_attributes);
    auto atom_with_no_custom_attributes = db->get_atom(node_with_no_custom_attributes->handle());
    EXPECT_EQ(atom_with_no_custom_attributes->custom_attributes.empty(), true);
    EXPECT_EQ(db->delete_atom(node_with_no_custom_attributes->handle()), true);

    Properties custom_attributes(
        {{"key_string", "string"}, {"key_long", 1}, {"key_double", 1.55}, {"key_bool", true}});
    auto node_with_custom_attributes = new Node("Symbol", "NodeWithCustomAttributes", custom_attributes);
    db->add_node(node_with_custom_attributes);

    auto atom_with_custom_attributes = db->get_atom(node_with_custom_attributes->handle());

    const string* string_value =
        atom_with_custom_attributes->custom_attributes.get_ptr<string>("key_string");
    EXPECT_EQ(*string_value, "string");
    const long* long_value = atom_with_custom_attributes->custom_attributes.get_ptr<long>("key_long");
    EXPECT_EQ(*long_value, 1L);
    const double* double_value =
        atom_with_custom_attributes->custom_attributes.get_ptr<double>("key_double");
    EXPECT_EQ(*double_value, 1.55);
    const bool* bool_value = atom_with_custom_attributes->custom_attributes.get_ptr<bool>("key_bool");
    EXPECT_EQ(*bool_value, true);

    EXPECT_EQ(db->delete_atom(node_with_custom_attributes->handle()), true);

    // MongoDB does not support unsigned int.
    Properties custom_attributes_2({{"key_unsigned_int", 1U}});
    auto node_with_custom_attributes_2 =
        new Node("Symbol", "NodeWithCustomAttributes2", custom_attributes_2);
    try {
        db->add_node(node_with_custom_attributes_2);
    } catch (const exception& e) {
        EXPECT_EQ(db->node_exists(node_with_custom_attributes_2->handle()), false);
    }
}

TEST_F(RedisMongoDBTest, ReIndexPatterns) {
    // (Similarity * *)
    auto similarity_node = new Node("Symbol", "Similarity");
    auto similarity_pattern = new Link(
        "Expression", {similarity_node->handle(), Atom::WILDCARD_STRING, Atom::WILDCARD_STRING});
    auto similarity_link_schema = new LinkSchemaHandle(similarity_pattern->handle().c_str());
    auto handle_set = db->query_for_pattern(*similarity_link_schema);
    EXPECT_EQ(handle_set->size(), 14);

    // (Inheritance * *)
    auto inheritance_node = new Node("Symbol", "Inheritance");
    auto inheritance_pattern = new Link(
        "Expression", {inheritance_node->handle(), Atom::WILDCARD_STRING, Atom::WILDCARD_STRING});
    auto inheritance_link_schema = new LinkSchemaHandle(inheritance_pattern->handle().c_str());
    handle_set = db->query_for_pattern(*inheritance_link_schema);
    EXPECT_EQ(handle_set->size(), 12);

    // (OddLink * *)
    auto odd_link_node = new Node("Symbol", "OddLink");
    auto odd_link_pattern = new Link("Expression", {odd_link_node->handle(), Atom::WILDCARD_STRING});
    auto odd_link_schema = new LinkSchemaHandle(odd_link_pattern->handle().c_str());
    handle_set = db->query_for_pattern(*odd_link_schema);
    EXPECT_EQ(handle_set->size(), 9);

    // Flush Redis patterns indexes
    db->flush_redis_by_prefix("test_patterns");

    handle_set = db->query_for_pattern(*similarity_link_schema);
    EXPECT_EQ(handle_set->size(), 0);

    handle_set = db->query_for_pattern(*inheritance_link_schema);
    EXPECT_EQ(handle_set->size(), 0);

    handle_set = db->query_for_pattern(*odd_link_schema);
    EXPECT_EQ(handle_set->size(), 0);

    // Clear Redis patterns indexes and re-index them
    db->re_index_patterns();

    handle_set = db->query_for_pattern(*similarity_link_schema);
    EXPECT_EQ(handle_set->size(), 14);

    handle_set = db->query_for_pattern(*inheritance_link_schema);
    EXPECT_EQ(handle_set->size(), 12);

    handle_set = db->query_for_pattern(*odd_link_schema);
    EXPECT_EQ(handle_set->size(), 9);
}

TEST_F(RedisMongoDBTest, GetMatchingAtoms) {
    auto similarity_node = new Node("Symbol", "Similarity");
    auto human_node = new Node("Symbol", "\"human\"");
    auto monkey_node = new Node("Symbol", "\"monkey\"");

    auto matching_atoms = db->get_matching_atoms(false, *similarity_node);
    EXPECT_EQ(matching_atoms.size(), 1);
    matching_atoms = db->get_matching_atoms(true, *similarity_node);
    EXPECT_EQ(matching_atoms.size(), 0);

    auto link =
        new Link("Expression", {similarity_node->handle(), human_node->handle(), monkey_node->handle()});
    matching_atoms = db->get_matching_atoms(true, *link);
    EXPECT_EQ(matching_atoms.size(), 1);

    matching_atoms = db->get_matching_atoms(false, *link);
    EXPECT_EQ(matching_atoms.size(), 0);

    auto link_document = db->get_atom_document(link->handle());
    EXPECT_EQ(link_document->get_bool("is_toplevel"), true);

    auto all_nodes = db->get_filtered_documents(RedisMongoDB::MONGODB_NODES_COLLECTION_NAME, {}, {});
    auto all_links = db->get_filtered_documents(RedisMongoDB::MONGODB_LINKS_COLLECTION_NAME, {}, {});

    auto untyped_variable = new UntypedVariable("V1", true);
    matching_atoms = db->get_matching_atoms(false, *untyped_variable);
    // Nodes are is_toplevel = false
    EXPECT_EQ(matching_atoms.size(), all_nodes.size());
    matching_atoms = db->get_matching_atoms(true, *untyped_variable);
    // Links are is_toplevel = true
    EXPECT_EQ(matching_atoms.size(), all_links.size());

    auto test_node = new Node("Symbol", "\"test\"");
    db->add_node(test_node);

    bool is_toplevel = true;
    auto top_level_link =
        new Link("Expression",
                 {similarity_node->handle(), human_node->handle(), test_node->handle()},
                 is_toplevel);
    db->add_link(top_level_link);

    matching_atoms = db->get_matching_atoms(is_toplevel, *top_level_link);
    EXPECT_EQ(matching_atoms.size(), 1);

    auto top_level_link_document = db->get_atom_document(top_level_link->handle());
    EXPECT_EQ(top_level_link_document->get_bool("is_toplevel"), is_toplevel);

    EXPECT_EQ(db->delete_atom(test_node->handle()), true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new RedisMongoDBTestEnvironment());
    return RUN_ALL_TESTS();
}
