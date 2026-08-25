#include <gtest/gtest.h>

#include <atomic>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <thread>
#include <vector>

#include "Atom.h"
#include "AtomDBFactory.h"
#include "AtomDBSingleton.h"
#include "Hasher.h"
#include "Link.h"
#include "Merger.h"
#include "MettaMapping.h"
#include "MockAnimalsData.h"
#include "Node.h"
#include "RedisMongoDB.h"
#include "TestAtomDBJsonConfig.h"
#include "UntypedVariable.h"
#include "Wildcard.h"
#include "expression_hasher.h"

using namespace atomdb;
using namespace atomdb::atomdb_api_types;
using namespace atoms;
using namespace std;

namespace {

string protection_config_document_id() {
    return Hasher::plain_string_hash(RedisMongoDB::MONGODB_CONFIG_COLLECTION_NAME);
}

template <typename Database>
bool mongodb_collection_exists(const Database& database, const string& name) {
    for (auto&& collection_info : database.list_collections()) {
        if (string(collection_info["name"].get_string().value) == name) {
            return true;
        }
    }
    return false;
}

}  // namespace

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
        auto atomdb = AtomDBFactory::create(test_atomdb_json_config(), "test_");
        ASSERT_NE(dynamic_pointer_cast<RedisMongoDB>(atomdb), nullptr);
        AtomDBSingleton::provide(atomdb);
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

class TestRedisMongoDB : public RedisMongoDB {
   public:
    TestRedisMongoDB(const string& context, const JsonConfig& config)
        : RedisMongoDB(context, true, config) {}
};

TEST_F(RedisMongoDBTest, ConcurrentQueryForPattern) {
    const int num_threads = 4;
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
    const int num_threads = 4;
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
    const int num_threads = 4;
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
    const int num_threads = 4;
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
    const int num_threads = 4;
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
    const int num_threads = 4;
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
    const int num_threads = 4;
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

TEST_F(RedisMongoDBTest, AddGetAndDeleteNode) {
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

    auto fetched_atom = db->get_atom(handle);
    auto fetched_node = db->get_node(handle);

    ASSERT_NE(fetched_atom, nullptr);
    ASSERT_NE(fetched_node, nullptr);

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

TEST_F(RedisMongoDBTest, AddGetAndDeleteLink) {
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

    auto fetched_atom = db->get_atom(handle);
    auto fetched_link = db->get_link(handle);

    ASSERT_NE(fetched_atom, nullptr);
    ASSERT_NE(fetched_link, nullptr);

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
    string named_type = matching_atoms[0]->type;
    EXPECT_EQ(named_type, string("Symbol"));
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

TEST_F(RedisMongoDBTest, UpdateAtom) {
    vector<Node*> nodes;
    nodes.push_back(new Node("Symbol", "UpdateAtom1"));
    nodes.push_back(new Node("Symbol", "UpdateAtom2"));
    nodes.push_back(new Node("Symbol", "UpdateAtom3"));

    vector<string> node_handles = db->add_nodes(nodes);
    EXPECT_EQ(node_handles.size(), 3);

    // Node 1 has no custom attributes
    auto node1_document =
        dynamic_pointer_cast<atomdb_api_types::MongodbDocument>(db->get_atom_document(node_handles[0]));
    EXPECT_EQ(node1_document->contains("custom_attributes"), false);

    auto custom_attributes = Properties();
    custom_attributes["field1"] = string("value1");

    // Update Node 1 with custom attributes
    string updated_node = db->add_node(new Node("Symbol", "UpdateAtom1", custom_attributes));
    node1_document =
        dynamic_pointer_cast<atomdb_api_types::MongodbDocument>(db->get_atom_document(updated_node));
    auto extracted_custom_attributes =
        node1_document->extract_custom_attributes(node1_document->get_object("custom_attributes"));
    EXPECT_EQ(extracted_custom_attributes.get<string>("field1"), string("value1"));

    auto link = new Link("Expression", node_handles, custom_attributes);
    string link_handle = db->add_link(link);

    auto atom_document =
        dynamic_pointer_cast<atomdb_api_types::MongodbDocument>(db->get_atom_document(link_handle));
    extracted_custom_attributes =
        atom_document->extract_custom_attributes(atom_document->get_object("custom_attributes"));
    EXPECT_EQ(extracted_custom_attributes.get<string>("field1"), string("value1"));

    // Update Link with modified custom attributes
    custom_attributes["field1"] = string("value2");
    link = new Link("Expression", node_handles, custom_attributes);
    link_handle = db->add_link(link);

    atom_document =
        dynamic_pointer_cast<atomdb_api_types::MongodbDocument>(db->get_atom_document(link_handle));
    extracted_custom_attributes =
        atom_document->extract_custom_attributes(atom_document->get_object("custom_attributes"));
    EXPECT_EQ(extracted_custom_attributes.get<string>("field1"), string("value2"));

    EXPECT_EQ(db->delete_atom(link_handle, true), true);

    EXPECT_EQ(db->link_exists(link_handle), false);
    EXPECT_EQ(db->node_exists(node_handles[0]), false);
    EXPECT_EQ(db->node_exists(node_handles[1]), false);
    EXPECT_EQ(db->node_exists(node_handles[2]), false);
}

TEST_F(RedisMongoDBTest, AddSameAtomMustNotThrow) {
    vector<Node*> nodes;
    nodes.push_back(new Node("Symbol", "AddSameAtomMustNotThrowNode1"));
    nodes.push_back(new Node("Symbol", "AddSameAtomMustNotThrowNode2"));
    nodes.push_back(new Node("Symbol", "AddSameAtomMustNotThrowNode3"));
    EXPECT_EQ(db->add_nodes(nodes).size(), 3);
    EXPECT_EQ(db->add_nodes(nodes).size(), 3);

    EXPECT_EQ(db->add_node(nodes[0]), nodes[0]->handle());
    EXPECT_EQ(db->add_node(nodes[0]), nodes[0]->handle());

    auto link = new Link("Expression", {nodes[0]->handle(), nodes[1]->handle(), nodes[2]->handle()});
    EXPECT_EQ(db->add_link(link), link->handle());
    EXPECT_EQ(db->add_link(link), link->handle());

    EXPECT_EQ(db->add_links({link, link}).size(), 2);
    EXPECT_EQ(db->add_links({link, link}).size(), 2);

    EXPECT_EQ(db->delete_link(link->handle(), true), true);

    EXPECT_EQ(db->link_exists(link->handle()), false);
    for (auto node : nodes) {
        EXPECT_EQ(db->node_exists(node->handle()), false);
    }
}

TEST_F(RedisMongoDBTest, AddNodesWithThrowIfExists) {
    auto node1 = new Node("Symbol", "ThrowIfExists1");
    EXPECT_EQ(db->add_node(node1, &ThrowIfExistsMerger::instance()), node1->handle());

    vector<Node*> nodes;
    nodes.push_back(new Node("Symbol", "ThrowIfExists2"));
    nodes.push_back(new Node("Symbol", "ThrowIfExists3"));

    EXPECT_EQ(db->add_nodes(nodes, false, &ThrowIfExistsMerger::instance()).size(), 2);

    auto link = new Link("Expression", {node1->handle(), nodes[0]->handle(), nodes[1]->handle()});
    EXPECT_EQ(db->add_link(link, &ThrowIfExistsMerger::instance()), link->handle());

    // Try to add the same node again
    EXPECT_THROW(db->add_node(node1, &ThrowIfExistsMerger::instance()), runtime_error);
    EXPECT_THROW(db->add_nodes(nodes, false, &ThrowIfExistsMerger::instance()), runtime_error);
    EXPECT_THROW(db->add_link(link, &ThrowIfExistsMerger::instance()), runtime_error);

    EXPECT_EQ(db->delete_link(link->handle(), true), true);
    EXPECT_EQ(db->link_exists(link->handle()), false);
    EXPECT_EQ(db->node_exists(node1->handle()), false);
    EXPECT_EQ(db->node_exists(nodes[0]->handle()), false);
    EXPECT_EQ(db->node_exists(nodes[1]->handle()), false);
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

TEST_F(RedisMongoDBTest, AddNodeReplacesByDefault) {
    Properties attrs1;
    attrs1["strength"] = 0.1;
    auto node1 = new Node("Symbol", "ReplaceByDefault1", attrs1);
    string handle = db->add_node(node1);
    EXPECT_DOUBLE_EQ(db->get_node(handle)->custom_attributes.get_or<double>("strength", -1.0), 0.1);

    Properties attrs2;
    attrs2["strength"] = 0.9;
    auto node2 = new Node("Symbol", "ReplaceByDefault1", attrs2);
    EXPECT_EQ(db->add_node(node2), handle);
    EXPECT_DOUBLE_EQ(db->get_node(handle)->custom_attributes.get_or<double>("strength", -1.0), 0.9);

    EXPECT_TRUE(db->delete_node(handle));
    delete node1;
    delete node2;
}

TEST_F(RedisMongoDBTest, AddNodeCustomMerger) {
    Properties attrs1;
    attrs1["strength"] = 0.2;
    auto node1 = new Node("Symbol", "CustomMerge1", attrs1);
    string handle = db->add_node(node1);

    Properties attrs2;
    attrs2["strength"] = 0.3;
    auto node2 = new Node("Symbol", "CustomMerge1", attrs2);
    SumStrengthMerger merger;
    EXPECT_EQ(db->add_node(node2, &merger), handle);
    EXPECT_DOUBLE_EQ(db->get_node(handle)->custom_attributes.get_or<double>("strength", -1.0), 0.5);

    EXPECT_TRUE(db->delete_node(handle));
    delete node1;
    delete node2;
}

TEST_F(RedisMongoDBTest, AddLinkReplacesAndCustomMerger) {
    auto n1 = new Node("Symbol", "LinkMergeA");
    auto n2 = new Node("Symbol", "LinkMergeB");
    auto h1 = db->add_node(n1);
    auto h2 = db->add_node(n2);

    Properties attrs1;
    attrs1["strength"] = 0.1;
    auto link1 = new Link("Expression", {h1, h2}, attrs1);
    string handle = db->add_link(link1);
    EXPECT_DOUBLE_EQ(db->get_link(handle)->custom_attributes.get_or<double>("strength", -1.0), 0.1);

    Properties attrs2;
    attrs2["strength"] = 0.9;
    auto link2 = new Link("Expression", {h1, h2}, attrs2);
    EXPECT_EQ(db->add_link(link2), handle);
    EXPECT_DOUBLE_EQ(db->get_link(handle)->custom_attributes.get_or<double>("strength", -1.0), 0.9);

    Properties attrs3;
    attrs3["strength"] = 0.25;
    auto link3 = new Link("Expression", {h1, h2}, attrs3);
    SumStrengthMerger merger;
    EXPECT_EQ(db->add_link(link3, &merger), handle);
    EXPECT_DOUBLE_EQ(db->get_link(handle)->custom_attributes.get_or<double>("strength", -1.0), 1.15);

    EXPECT_TRUE(db->delete_link(handle, true));
    delete n1;
    delete n2;
    delete link1;
    delete link2;
    delete link3;
}

TEST_F(RedisMongoDBTest, AddNodesBatchReplaceAndCustomMerger) {
    Properties attrs1;
    attrs1["strength"] = 0.1;
    attrs1["obsolete"] = true;
    Properties attrs2;
    attrs2["strength"] = 0.4;
    Properties attrs3;
    attrs3["strength"] = 0.5;

    auto first_a = new Node("Symbol", "BatchNodeA", attrs1);
    auto first_b = new Node("Symbol", "BatchNodeB", attrs1);
    EXPECT_EQ(db->add_nodes({first_a, first_b}).size(), 2u);
    EXPECT_TRUE(db->get_node(first_a->handle())->custom_attributes.get_or<bool>("obsolete", false));

    auto replace_a = new Node("Symbol", "BatchNodeA", attrs2);
    auto replace_b = new Node("Symbol", "BatchNodeB", attrs2);
    EXPECT_EQ(db->add_nodes({replace_a, replace_b}).size(), 2u);
    EXPECT_DOUBLE_EQ(
        db->get_node(replace_a->handle())->custom_attributes.get_or<double>("strength", -1.0), 0.4);
    EXPECT_FALSE(db->get_node(replace_a->handle())->custom_attributes.get_or<bool>("obsolete", false));

    auto merge_a1 = new Node("Symbol", "BatchNodeA", attrs3);
    auto merge_a2 = new Node("Symbol", "BatchNodeA", attrs3);
    SumStrengthMerger merger;
    EXPECT_EQ(db->add_nodes({merge_a1, merge_a2}, false, &merger).size(), 2u);
    // 0.4 + 0.5 + 0.5 from in-batch repeated handle
    EXPECT_DOUBLE_EQ(
        db->get_node(merge_a1->handle())->custom_attributes.get_or<double>("strength", -1.0), 1.4);

    EXPECT_TRUE(db->delete_node(first_a->handle()));
    EXPECT_TRUE(db->delete_node(first_b->handle()));
    delete first_a;
    delete first_b;
    delete replace_a;
    delete replace_b;
    delete merge_a1;
    delete merge_a2;
}

TEST_F(RedisMongoDBTest, AddLinksBatchReplaceAndCustomMerger) {
    auto n1 = new Node("Symbol", "BatchLinkN1");
    auto n2 = new Node("Symbol", "BatchLinkN2");
    auto h1 = db->add_node(n1);
    auto h2 = db->add_node(n2);

    Properties attrs1;
    attrs1["strength"] = 0.1;
    attrs1["obsolete"] = true;
    Properties attrs2;
    attrs2["strength"] = 0.4;
    Properties attrs3;
    attrs3["strength"] = 0.5;

    auto link1 = new Link("Expression", {h1, h2}, attrs1);
    EXPECT_EQ(db->add_links({link1}).size(), 1u);

    auto link2 = new Link("Expression", {h1, h2}, attrs2);
    EXPECT_EQ(db->add_links({link2}).size(), 1u);
    EXPECT_DOUBLE_EQ(db->get_link(link2->handle())->custom_attributes.get_or<double>("strength", -1.0),
                     0.4);
    EXPECT_FALSE(db->get_link(link2->handle())->custom_attributes.get_or<bool>("obsolete", false));

    auto link3a = new Link("Expression", {h1, h2}, attrs3);
    auto link3b = new Link("Expression", {h1, h2}, attrs3);
    SumStrengthMerger merger;
    EXPECT_EQ(db->add_links({link3a, link3b}, false, &merger).size(), 2u);
    EXPECT_DOUBLE_EQ(db->get_link(link3a->handle())->custom_attributes.get_or<double>("strength", -1.0),
                     1.4);

    EXPECT_TRUE(db->delete_link(link1->handle(), true));
    delete n1;
    delete n2;
    delete link1;
    delete link2;
    delete link3a;
    delete link3b;
}

TEST_F(RedisMongoDBTest, AddLinksWithDuplicateTargets) {
    vector<Node*> nodes;
    nodes.push_back(new Node("Symbol", "DuplicateTargets1"));
    nodes.push_back(new Node("Symbol", "DuplicateTargets2"));
    nodes.push_back(new Node("Symbol", "DuplicateTargets3"));
    EXPECT_EQ(db->add_nodes(nodes, false, &ThrowIfExistsMerger::instance()).size(), 3);

    auto link = new Link("Expression",
                         {nodes[0]->handle(),
                          nodes[1]->handle(),
                          nodes[1]->handle(),
                          nodes[0]->handle(),
                          nodes[2]->handle(),
                          nodes[0]->handle(),
                          nodes[2]->handle()});
    EXPECT_EQ(db->add_link(link), link->handle());
    EXPECT_EQ(db->delete_link(link->handle(), true), true);
}

TEST_F(RedisMongoDBTest, CompositeHashDisabledSkipsTargetChecks) {
    // Default (composite_type_enabled=true) rejects links whose targets are missing locally.
    auto missing_a = new Node("Symbol", "MissingCompositeTargetA");
    auto missing_b = new Node("Symbol", "MissingCompositeTargetB");
    auto implication = new Node("Symbol", "ImplicationMissingComposite");
    auto link = new Link("Expression",
                         {implication->handle(), missing_a->handle(), missing_b->handle()},
                         true,
                         Properties{{"strength", 0.833333}});
    EXPECT_THROW(db->add_links({link}), runtime_error);

    // Same Redis/Mongo namespace, composite_type_enabled=false: skip checks and persist the link.
    auto config = test_atomdb_json_config();
    config["composite_type_enabled"] = false;
    auto local_db = dynamic_pointer_cast<RedisMongoDB>(AtomDBFactory::create(config, "test_"));
    ASSERT_NE(local_db, nullptr);

    auto handles = local_db->add_links({link});
    ASSERT_EQ(handles.size(), 1);
    EXPECT_EQ(handles[0], link->handle());
    ASSERT_TRUE(local_db->link_exists(link->handle()));
    EXPECT_FALSE(local_db->atom_exists(missing_a->handle()));
    EXPECT_FALSE(local_db->atom_exists(missing_b->handle()));

    auto got = local_db->get_atom(link->handle());
    ASSERT_NE(got, nullptr);
    EXPECT_DOUBLE_EQ(got->custom_attributes.get_or<double>("strength", -1.0), 0.833333);

    EXPECT_TRUE(local_db->delete_link(link->handle(), false));
    delete link;
    delete missing_a;
    delete missing_b;
    delete implication;
}

TEST_F(RedisMongoDBTest, AtomsCount) {
    db->drop_all();

    EXPECT_EQ(db->node_count(), 0);
    EXPECT_EQ(db->link_count(), 0);
    EXPECT_EQ(db->atom_count(), 0);
    EXPECT_EQ(db->empty(), true);

    auto node1 = new Node("Symbol", "Node1");
    auto node2 = new Node("Symbol", "Node2");
    auto similarity = new Node("Symbol", "Similarity");

    db->add_node(node1);
    db->add_node(node2);
    db->add_node(similarity);

    EXPECT_EQ(db->node_count(), 3);
    EXPECT_EQ(db->link_count(), 0);
    EXPECT_EQ(db->atom_count(), 3);
    EXPECT_EQ(db->empty(), false);

    auto link1 = new Link("Expression", {similarity->handle(), node1->handle(), node2->handle()});
    db->add_link(link1);

    EXPECT_EQ(db->node_count(), 3);
    EXPECT_EQ(db->link_count(), 1);
    EXPECT_EQ(db->atom_count(), 4);
    EXPECT_EQ(db->empty(), false);
}

TEST_F(RedisMongoDBTest, CompositeTypeEnabledFlag) {
    EXPECT_TRUE(db->composite_type_enabled());

    auto config_default = test_atomdb_json_config();
    config_default.erase("composite_type_enabled");
    auto db_default = dynamic_pointer_cast<RedisMongoDB>(AtomDBFactory::create(config_default, "test_"));
    ASSERT_NE(db_default, nullptr);
    EXPECT_TRUE(db_default->composite_type_enabled());

    vector<Node*> enabled_nodes = {new Node("Symbol", "CompositeTypeEnabled-A"),
                                   new Node("Symbol", "CompositeTypeEnabled-B"),
                                   new Node("Symbol", "CompositeTypeEnabled-C")};
    ASSERT_EQ(db->add_nodes(enabled_nodes).size(), 3);
    auto enabled_link =
        new Link("Expression",
                 {enabled_nodes[0]->handle(), enabled_nodes[1]->handle(), enabled_nodes[2]->handle()});
    string enabled_link_handle = db->add_link(enabled_link);
    ASSERT_FALSE(enabled_link_handle.empty());

    auto enabled_doc = db->get_atom_document(enabled_link_handle);
    ASSERT_NE(enabled_doc, nullptr);
    EXPECT_TRUE(enabled_doc->contains("composite_type_hash"));
    EXPECT_TRUE(enabled_doc->contains("composite_type"));
    EXPECT_EQ(string(enabled_doc->get("composite_type_hash")), enabled_link->composite_type_hash(*db));
    EXPECT_EQ(enabled_doc->get_size("composite_type"), 4);

    auto config_disabled = test_atomdb_json_config();
    config_disabled["composite_type_enabled"] = false;
    auto db_disabled =
        dynamic_pointer_cast<RedisMongoDB>(AtomDBFactory::create(config_disabled, "test_"));
    ASSERT_NE(db_disabled, nullptr);
    EXPECT_FALSE(db_disabled->composite_type_enabled());

    vector<Node*> disabled_nodes = {new Node("Symbol", "CompositeTypeDisabled-A"),
                                    new Node("Symbol", "CompositeTypeDisabled-B"),
                                    new Node("Symbol", "CompositeTypeDisabled-C")};
    ASSERT_EQ(db_disabled->add_nodes(disabled_nodes).size(), 3);
    auto disabled_link = new Link(
        "Expression",
        {disabled_nodes[0]->handle(), disabled_nodes[1]->handle(), disabled_nodes[2]->handle()});
    string disabled_link_handle = db_disabled->add_link(disabled_link);
    ASSERT_FALSE(disabled_link_handle.empty());

    auto disabled_doc = db_disabled->get_atom_document(disabled_link_handle);
    ASSERT_NE(disabled_doc, nullptr);
    EXPECT_FALSE(disabled_doc->contains("composite_type_hash"));
    EXPECT_FALSE(disabled_doc->contains("composite_type"));

    vector<Node*> transactional_nodes = {new Node("Symbol", "CompositeTypeDisabledTx-A"),
                                         new Node("Symbol", "CompositeTypeDisabledTx-B"),
                                         new Node("Symbol", "CompositeTypeDisabledTx-C")};
    auto transactional_link = new Link("Expression",
                                       {transactional_nodes[0]->handle(),
                                        transactional_nodes[1]->handle(),
                                        transactional_nodes[2]->handle()});
    ASSERT_EQ(db_disabled->add_nodes(transactional_nodes, true).size(), 3);
    ASSERT_EQ(db_disabled->add_links({transactional_link}, true).size(), 1);

    auto transactional_doc = db_disabled->get_atom_document(transactional_link->handle());
    ASSERT_NE(transactional_doc, nullptr);
    EXPECT_FALSE(transactional_doc->contains("composite_type_hash"));
    EXPECT_FALSE(transactional_doc->contains("composite_type"));

    EXPECT_TRUE(db->delete_atom(enabled_link_handle, true));
    EXPECT_TRUE(db_disabled->delete_atom(disabled_link_handle, true));
    EXPECT_TRUE(db_disabled->delete_atom(transactional_link->handle(), true));
}

TEST_F(RedisMongoDBTest, TransactionalMergeUsesFinalLinkCompositeType) {
    vector<Node*> nodes = {new Node("Symbol", "TxMergeCT-A"),
                           new Node("Symbol", "TxMergeCT-B"),
                           new Node("Symbol", "TxMergeCT-C")};
    ASSERT_EQ(db->add_nodes(nodes, true).size(), 3u);

    Properties attrs1;
    attrs1["strength"] = 0.2;
    auto link1 =
        new Link("Expression", {nodes[0]->handle(), nodes[1]->handle(), nodes[2]->handle()}, attrs1);
    ASSERT_EQ(db->add_links({link1}, true).size(), 1u);

    string expected_hash = link1->composite_type_hash(*db);
    auto doc1 = db->get_atom_document(link1->handle());
    ASSERT_NE(doc1, nullptr);
    EXPECT_EQ(string(doc1->get("composite_type_hash")), expected_hash);
    EXPECT_EQ(doc1->get_size("composite_type"), 4);

    Properties attrs2;
    attrs2["strength"] = 0.3;
    auto link2 =
        new Link("Expression", {nodes[0]->handle(), nodes[1]->handle(), nodes[2]->handle()}, attrs2);
    SumStrengthMerger merger;
    ASSERT_EQ(db->add_nodes(nodes, true).size(), 3u);
    ASSERT_EQ(db->add_links({link2}, true, &merger).size(), 1u);

    auto doc2 = db->get_atom_document(link2->handle());
    ASSERT_NE(doc2, nullptr);
    EXPECT_EQ(string(doc2->get("composite_type_hash")), expected_hash);
    EXPECT_EQ(doc2->get_size("composite_type"), 4);
    EXPECT_DOUBLE_EQ(db->get_link(link2->handle())->custom_attributes.get_or<double>("strength", -1.0),
                     0.5);

    EXPECT_TRUE(db->delete_atom(link2->handle(), true));
    delete nodes[0];
    delete nodes[1];
    delete nodes[2];
    delete link1;
    delete link2;
}

TEST_F(RedisMongoDBTest, TransactionalRejectedMergeStillBooksCompositeType) {
    vector<Node*> nodes = {new Node("Symbol", "TxRejectCT-A"),
                           new Node("Symbol", "TxRejectCT-B"),
                           new Node("Symbol", "TxRejectCT-C"),
                           new Node("Symbol", "TxRejectCT-D"),
                           new Node("Symbol", "TxRejectCT-E")};
    ASSERT_EQ(db->add_nodes(nodes, true).size(), 5u);

    auto existing = new Link("Expression", {nodes[0]->handle(), nodes[1]->handle(), nodes[2]->handle()});
    ASSERT_EQ(db->add_links({existing}, true).size(), 1u);
    string existing_hash = existing->composite_type_hash(*db);

    // duplicate is rejected by SkipIfExistsMerger; nested targets the rejected handle and must
    // still resolve its composite-type bookkeeping in the same transactional batch.
    auto duplicate =
        new Link("Expression", {nodes[0]->handle(), nodes[1]->handle(), nodes[2]->handle()});
    auto nested = new Link("Expression", {existing->handle(), nodes[3]->handle(), nodes[4]->handle()});

    ASSERT_EQ(db->add_nodes(nodes, true).size(), 5u);
    auto handles = db->add_links({duplicate, nested}, true, &SkipIfExistsMerger::instance());
    ASSERT_EQ(handles.size(), 2u);
    EXPECT_EQ(handles[0], "");
    EXPECT_EQ(handles[1], nested->handle());

    auto existing_doc = db->get_atom_document(existing->handle());
    ASSERT_NE(existing_doc, nullptr);
    EXPECT_EQ(string(existing_doc->get("composite_type_hash")), existing_hash);

    auto nested_doc = db->get_atom_document(nested->handle());
    ASSERT_NE(nested_doc, nullptr);
    ASSERT_EQ(nested_doc->get_size("composite_type"), 4u);
    // Transactional map stores named_type_hash for link targets (same as
    // build_composite_type_entries_map).
    EXPECT_EQ(string(nested_doc->get("composite_type", 0)), nested->named_type_hash());
    EXPECT_EQ(string(nested_doc->get("composite_type", 1)), existing->named_type_hash());
    EXPECT_FALSE(string(nested_doc->get("composite_type", 1)).empty());
    EXPECT_EQ(string(nested_doc->get("composite_type", 2)), nodes[3]->named_type_hash());
    EXPECT_EQ(string(nested_doc->get("composite_type", 3)), nodes[4]->named_type_hash());
    EXPECT_EQ(string(nested_doc->get("composite_type_hash")),
              Hasher::composite_handle({nested->named_type_hash(),
                                        existing->named_type_hash(),
                                        nodes[3]->named_type_hash(),
                                        nodes[4]->named_type_hash()}));

    EXPECT_TRUE(db->delete_atom(nested->handle(), false));
    EXPECT_TRUE(db->delete_atom(existing->handle(), true));
    EXPECT_TRUE(db->delete_node(nodes[3]->handle()));
    EXPECT_TRUE(db->delete_node(nodes[4]->handle()));
    for (auto* node : nodes) {
        delete node;
    }
    delete existing;
    delete duplicate;
    delete nested;
}

TEST_F(RedisMongoDBTest, GetAccessPermissionsEmptyCollection) {
    auto pool = db->get_mongo_pool();
    auto conn = pool->acquire();
    auto collection =
        (*conn)[RedisMongoDB::MONGODB_DB_NAME][RedisMongoDB::MONGODB_ACCESS_PERMISSIONS_COLLECTION_NAME];
    collection.delete_many({});

    auto permissions = db->get_access_permissions(PublicKey("any_key"));
    EXPECT_TRUE(permissions.empty());
}

TEST_F(RedisMongoDBTest, GetAccessPermissionsReturnsStoredDocuments) {
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_array;
    using bsoncxx::builder::basic::make_document;

    auto pool = db->get_mongo_pool();
    auto conn = pool->acquire();
    auto collection =
        (*conn)[RedisMongoDB::MONGODB_DB_NAME][RedisMongoDB::MONGODB_ACCESS_PERMISSIONS_COLLECTION_NAME];
    collection.delete_many({});

    auto similarity_tokens = make_array(
        "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Similarity", "VARIABLE", "VARIABLE");
    string admin_id = compute_hash((char*) "key_admin");
    string reader_id = compute_hash((char*) "key_reader");
    collection.insert_one(make_document(kvp("_id", admin_id),
                                        kvp("public_key", "key_admin"),
                                        kvp("full_access", true),
                                        kvp("allowed_schemas", make_array())));
    collection.insert_one(make_document(
        kvp("_id", reader_id),
        kvp("public_key", "key_reader"),
        kvp("full_access", false),
        kvp("allowed_schemas",
            make_array(make_document(
                kvp("tokens", similarity_tokens), kvp("read", true), kvp("write", false))))));

    auto admin_permissions = db->get_access_permissions(PublicKey("key_admin"));
    ASSERT_EQ(admin_permissions.size(), 1u);
    EXPECT_STREQ(admin_permissions[0]->get_access_key(), "key_admin");
    EXPECT_TRUE(admin_permissions[0]->get_full_access());
    EXPECT_EQ(admin_permissions[0]->get_entries_size(), 0u);

    auto reader_permissions = db->get_access_permissions(PublicKey("key_reader"));
    ASSERT_EQ(reader_permissions.size(), 1u);
    EXPECT_STREQ(reader_permissions[0]->get_access_key(), "key_reader");
    EXPECT_FALSE(reader_permissions[0]->get_full_access());
    ASSERT_EQ(reader_permissions[0]->get_entries_size(), 1u);
    const auto& reader_entry = reader_permissions[0]->get_entry(0);
    EXPECT_TRUE(reader_entry.get_read());
    EXPECT_FALSE(reader_entry.get_write());
    vector<string> expected_tokens = {
        "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Similarity", "VARIABLE", "VARIABLE"};
    LinkSchema expected_schema(expected_tokens);
    vector<string> actual_tokens;
    actual_tokens.reserve(reader_entry.get_tokens_size());
    for (unsigned int i = 0; i < reader_entry.get_tokens_size(); ++i) {
        actual_tokens.push_back(reader_entry.get_token(i));
    }
    EXPECT_EQ(LinkSchema(actual_tokens).handle(), expected_schema.handle());

    auto map_permissions = db->get_access_permissions(
        PublicKey(map<string, string>{{"peer_a", "key_admin"}, {"peer_b", "key_reader"}}));
    ASSERT_EQ(map_permissions.size(), 2u);

    EXPECT_TRUE(db->get_access_permissions(PublicKey("missing_key")).empty());

    collection.delete_many({});
    EXPECT_TRUE(db->get_access_permissions(PublicKey("key_admin")).empty());
}

TEST_F(RedisMongoDBTest, GetAccessPermissionsRejectsInvalidDocument) {
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    auto pool = db->get_mongo_pool();
    auto conn = pool->acquire();
    auto collection =
        (*conn)[RedisMongoDB::MONGODB_DB_NAME][RedisMongoDB::MONGODB_ACCESS_PERMISSIONS_COLLECTION_NAME];
    collection.delete_many({});

    collection.insert_one(make_document(kvp("_id", string(compute_hash((char*) "key_broken"))),
                                        kvp("full_access", false)));

    EXPECT_THROW(db->get_access_permissions(PublicKey("key_broken")), runtime_error);

    collection.delete_many({});
}

TEST_F(RedisMongoDBTest, IsProtectedWhenPersistedConfigIsTrue) {
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    auto conn = db->get_mongo_pool()->acquire();
    auto collection =
        (*conn)[RedisMongoDB::MONGODB_DB_NAME][RedisMongoDB::MONGODB_CONFIG_COLLECTION_NAME];
    collection.delete_many({});
    collection.insert_one(
        make_document(kvp("_id", protection_config_document_id()), kvp("protected", true)));

    TestRedisMongoDB loaded("test_", test_atomdb_json_config());
    EXPECT_EQ(loaded.get_protection_mode(), atomdb_api_types::ProtectionMode::PROTECTED);

    collection.delete_many({});
}

TEST_F(RedisMongoDBTest, IsUnprotectedWhenPersistedConfigIsFalse) {
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    auto conn = db->get_mongo_pool()->acquire();
    auto collection =
        (*conn)[RedisMongoDB::MONGODB_DB_NAME][RedisMongoDB::MONGODB_CONFIG_COLLECTION_NAME];
    collection.delete_many({});
    collection.insert_one(
        make_document(kvp("_id", protection_config_document_id()), kvp("protected", false)));

    TestRedisMongoDB loaded("test_", test_atomdb_json_config());
    EXPECT_EQ(loaded.get_protection_mode(), atomdb_api_types::ProtectionMode::UNPROTECTED);

    collection.delete_many({});
}

TEST_F(RedisMongoDBTest, IsUnprotectedWhenPersistedConfigDocumentAbsent) {
    auto conn = db->get_mongo_pool()->acquire();
    auto collection =
        (*conn)[RedisMongoDB::MONGODB_DB_NAME][RedisMongoDB::MONGODB_CONFIG_COLLECTION_NAME];
    collection.delete_many({});

    TestRedisMongoDB loaded("test_", test_atomdb_json_config());
    EXPECT_EQ(loaded.get_protection_mode(), atomdb_api_types::ProtectionMode::UNPROTECTED);
}

TEST_F(RedisMongoDBTest, RejectsPersistedConfigMissingProtectedField) {
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    auto conn = db->get_mongo_pool()->acquire();
    auto collection =
        (*conn)[RedisMongoDB::MONGODB_DB_NAME][RedisMongoDB::MONGODB_CONFIG_COLLECTION_NAME];
    collection.delete_many({});
    collection.insert_one(
        make_document(kvp("_id", protection_config_document_id()), kvp("other", "value")));

    EXPECT_THROW({ TestRedisMongoDB loaded("test_", test_atomdb_json_config()); }, runtime_error);

    collection.delete_many({});
}

TEST_F(RedisMongoDBTest, RejectsPersistedConfigInvalidProtectedFieldType) {
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    auto conn = db->get_mongo_pool()->acquire();
    auto collection =
        (*conn)[RedisMongoDB::MONGODB_DB_NAME][RedisMongoDB::MONGODB_CONFIG_COLLECTION_NAME];
    collection.delete_many({});
    collection.insert_one(
        make_document(kvp("_id", protection_config_document_id()), kvp("protected", "yes")));

    EXPECT_THROW({ TestRedisMongoDB loaded("test_", test_atomdb_json_config()); }, runtime_error);

    collection.delete_many({});
}

TEST_F(RedisMongoDBTest, DropAllDropsEveryCollectionExceptConfig) {
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    auto conn = db->get_mongo_pool()->acquire();
    auto database = (*conn)[RedisMongoDB::MONGODB_DB_NAME];
    auto config_collection = database[RedisMongoDB::MONGODB_CONFIG_COLLECTION_NAME];

    ASSERT_GT(database[RedisMongoDB::MONGODB_NODES_COLLECTION_NAME].count_documents({}), 0u);
    ASSERT_GT(database[RedisMongoDB::MONGODB_LINKS_COLLECTION_NAME].count_documents({}), 0u);

    config_collection.delete_many({});
    config_collection.insert_one(
        make_document(kvp("_id", protection_config_document_id()), kvp("protected", true)));

    db->drop_all();

    ASSERT_TRUE(mongodb_collection_exists(database, RedisMongoDB::MONGODB_CONFIG_COLLECTION_NAME));
    EXPECT_EQ(config_collection.count_documents({}), 1u);
    auto config_doc =
        config_collection.find_one(make_document(kvp("_id", protection_config_document_id())));
    ASSERT_TRUE(config_doc.has_value());
    EXPECT_TRUE(config_doc->view()["protected"].get_bool().value);

    EXPECT_FALSE(mongodb_collection_exists(database, RedisMongoDB::MONGODB_NODES_COLLECTION_NAME));
    EXPECT_FALSE(mongodb_collection_exists(database, RedisMongoDB::MONGODB_LINKS_COLLECTION_NAME));
    EXPECT_FALSE(
        mongodb_collection_exists(database, RedisMongoDB::MONGODB_PATTERN_INDEX_SCHEMA_COLLECTION_NAME));
    EXPECT_FALSE(
        mongodb_collection_exists(database, RedisMongoDB::MONGODB_ACCESS_PERMISSIONS_COLLECTION_NAME));

    TestRedisMongoDB reloaded("test_", test_atomdb_json_config());
    EXPECT_EQ(reloaded.get_protection_mode(), atomdb_api_types::ProtectionMode::PROTECTED);

    config_collection.delete_many({});
}

TEST_F(RedisMongoDBTest, DropAllPreservesProtectionConfiguration) {
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    auto conn = db->get_mongo_pool()->acquire();
    auto collection =
        (*conn)[RedisMongoDB::MONGODB_DB_NAME][RedisMongoDB::MONGODB_CONFIG_COLLECTION_NAME];
    collection.delete_many({});
    collection.insert_one(
        make_document(kvp("_id", protection_config_document_id()), kvp("protected", true)));

    TestRedisMongoDB protected_db("test_", test_atomdb_json_config());
    ASSERT_EQ(protected_db.get_protection_mode(), atomdb_api_types::ProtectionMode::PROTECTED);

    protected_db.drop_all();

    EXPECT_EQ(protected_db.get_protection_mode(), atomdb_api_types::ProtectionMode::PROTECTED);

    TestRedisMongoDB reloaded("test_", test_atomdb_json_config());
    EXPECT_EQ(reloaded.get_protection_mode(), atomdb_api_types::ProtectionMode::PROTECTED);

    collection.delete_many({});
}

TEST_F(RedisMongoDBTest, IgnoresConflictingProtectionConfigurationDocuments) {
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    auto conn = db->get_mongo_pool()->acquire();
    auto collection =
        (*conn)[RedisMongoDB::MONGODB_DB_NAME][RedisMongoDB::MONGODB_CONFIG_COLLECTION_NAME];
    collection.delete_many({});

    collection.insert_one(
        make_document(kvp("_id", protection_config_document_id()), kvp("protected", true)));
    collection.insert_one(make_document(kvp("protected", false)));

    TestRedisMongoDB loaded("test_", test_atomdb_json_config());
    EXPECT_EQ(loaded.get_protection_mode(), atomdb_api_types::ProtectionMode::PROTECTED);

    collection.delete_many({});
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new RedisMongoDBTestEnvironment());
    return RUN_ALL_TESTS();
}
