#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>

#include "AtomDBSingleton.h"
#include "AtomSpace.h"
#include "Link.h"
#include "Node.h"
#include "Properties.h"
#include "ServiceBusSingleton.h"

using namespace std;
using namespace atomspace;
using namespace atoms;
using namespace commons;

// Mock AtomDocument for testing
class MockAtomDocument : public AtomDocument {
   private:
    map<string, string> values;
    map<string, vector<string>> arrays;

   public:
    void set_value(const string& key, const string& value) { values[key] = value; }

    void add_array_value(const string& key, const string& value) { arrays[key].push_back(value); }

    const char* get(const string& key) override {
        auto it = values.find(key);
        if (it != values.end()) {
            return it->second.c_str();
        }
        return nullptr;
    }

    const char* get(const string& key, unsigned int index) override {
        auto it = arrays.find(key);
        if (it != arrays.end() && index < it->second.size()) {
            return it->second[index].c_str();
        }
        return nullptr;
    }

    unsigned int get_size(const string& key) override {
        auto it = arrays.find(key);
        if (it != arrays.end()) {
            return it->second.size();
        }
        return 0;
    }

    bool contains(const string& key) override {
        return values.find(key) != values.end() || arrays.find(key) != arrays.end();
    }
};

// Mock AtomDB for testing
class MockAtomDB : public AtomDB {
   public:
    // Track method calls
    mutable vector<string> get_atom_calls;
    vector<tuple<string, string, Properties>> add_node_calls;
    vector<tuple<string, vector<string>, Properties>> add_link_calls;

    // Mock documents to return
    map<string, shared_ptr<AtomDocument>> docs;

    // AtomDB interface implementation
    shared_ptr<HandleSet> query_for_pattern(const LinkTemplateInterface&) override { return nullptr; }
    shared_ptr<HandleList> query_for_targets(const string&) override { return nullptr; }

    map<string, Atom*> atoms;

    shared_ptr<AtomDocument> get_atom_document(const string& handle) override {
        get_atom_calls.push_back(handle);
        auto it = docs.find(handle);
        if (it != docs.end()) {
            return it->second;
        }
        return nullptr;
    }

    vector<shared_ptr<AtomDocument>> get_atom_documents(const vector<string>& handles,
                                                        const vector<string>& fields) override {
        return {};  // Not implemented for this mock
    }

    bool link_exists(const string&) override { return false; }
    set<string> links_exist(const vector<string>&) override { return {}; }

    string add_node(const atoms::Node* node) override {
        add_node_calls.push_back({node->type, node->name, node->custom_attributes});
        // string handle = string("node_") + node->type + "_" + node->name;
        atoms[node->handle()] = (Atom*) node;
        return node->handle();
    }

    string add_link(const atoms::Link* link) override {
        add_link_calls.push_back({link->type, link->targets, link->custom_attributes});
        atoms[link->handle()] = (Atom*) link;
        return link->handle();
    }

    vector<string> add_nodes(const vector<atoms::Node*>& nodes) override { return {}; }
    vector<string> add_links(const vector<atoms::Link*>& links) override { return {}; }

    bool delete_node(const string& handle) override { return true; }
    uint delete_nodes(const vector<string>& handles) override { return 0; }

    bool delete_link(const string& handle) override { return true; }
    uint delete_links(const vector<string>& handles) override { return 0; }

    shared_ptr<Atom> get_atom(const string& handle) { return shared_ptr<Atom>(this->atoms[handle]); }

    void attention_broker_setup() override {}
};

class MockAtomSpace : public AtomSpace {
   public:
    MockAtomSpace(shared_ptr<MockAtomDB> db) : AtomSpace() { this->db = db; }
    Atom* atom_from_document(const shared_ptr<AtomDocument>& document) {
        return AtomSpace::atom_from_document(document);
    }
};

// Fixture for AtomSpace tests
class AtomSpaceTest : public ::testing::Test {
   protected:
    shared_ptr<MockAtomDB> mock_db;
    MockAtomSpace* space;

    void SetUp() override {
        mock_db = make_shared<MockAtomDB>();
        space = new MockAtomSpace(mock_db);
    }

    void TearDown() override { delete space; }

    // Helper to create a node document
    shared_ptr<MockAtomDocument> create_node_doc(const string& type, const string& name) {
        auto doc = make_shared<MockAtomDocument>();
        doc->set_value("type", type);
        doc->set_value("name", name);
        return doc;
    }

    // Helper to create a link document
    shared_ptr<MockAtomDocument> create_link_doc(const string& type, const vector<string>& targets) {
        auto doc = make_shared<MockAtomDocument>();
        doc->set_value("type", type);
        for (const auto& target : targets) {
            doc->add_array_value("targets", target);
        }
        return doc;
    }
};

// Test get_atom for local retrieval
TEST_F(AtomSpaceTest, GetAtomLocalOnly) {
    // Add a node to the local trie
    unique_ptr<char> handle(space->add_node("ConceptNode", "test"));

    // Retrieve it with LOCAL_ONLY scope
    const Atom* atom = space->get_atom(handle.get(), AtomSpace::LOCAL_ONLY);

    // Verify we got it back
    ASSERT_NE(atom, nullptr);
    EXPECT_EQ(atom->type, "ConceptNode");
    EXPECT_EQ(dynamic_cast<const Node*>(atom)->name, "test");

    // Verify we didn't call the DB
    EXPECT_TRUE(mock_db->get_atom_calls.empty());
}

// Test get_atom for remote retrieval
TEST_F(AtomSpaceTest, GetAtomRemoteOnly) {
    // Setup mock DB to return a document
    auto doc = create_node_doc("ConceptNode", "remote_test");
    auto handle = "277b74dac3774d0d866869b76caeb99c";
    mock_db->docs[handle] = doc;

    // Retrieve with REMOTE_ONLY scope
    const Atom* atom = space->get_atom(handle, AtomSpace::REMOTE_ONLY);

    // Verify we got the atom from DB
    ASSERT_NE(atom, nullptr);
    EXPECT_EQ(atom->type, "ConceptNode");
    EXPECT_EQ(dynamic_cast<const Node*>(atom)->name, "remote_test");

    // Verify we called the DB
    ASSERT_EQ(mock_db->get_atom_calls.size(), 1);
    EXPECT_EQ(mock_db->get_atom_calls[0], handle);
}

// Test add_node functionality
TEST_F(AtomSpaceTest, AddNode) {
    // Add a node
    unique_ptr<char> handle(space->add_node("ConceptNode", "test_node"));

    // Verify the node was added to local trie
    const Atom* atom = space->get_atom(handle.get(), AtomSpace::LOCAL_ONLY);
    ASSERT_NE(atom, nullptr);
    EXPECT_EQ(atom->type, "ConceptNode");
    EXPECT_EQ(dynamic_cast<const Node*>(atom)->name, "test_node");
    EXPECT_TRUE(AtomSpace::is_node(*atom));
    EXPECT_FALSE(AtomSpace::is_link(*atom));
}

// Test add_link functionality
TEST_F(AtomSpaceTest, AddLink) {
    // First add two nodes
    unique_ptr<char> node1_handle(space->add_node("ConceptNode", "node1"));
    unique_ptr<char> node2_handle(space->add_node("ConceptNode", "node2"));

    // Create a vector of target atoms
    vector<string> targets;
    targets.push_back(node1_handle.get());
    targets.push_back(node2_handle.get());

    // Add a link connecting these nodes
    unique_ptr<char> link_handle(space->add_link("ListLink", targets));

    // Verify the link was added to local trie
    const Atom* link_atom = space->get_atom(link_handle.get(), AtomSpace::LOCAL_ONLY);
    ASSERT_NE(link_atom, nullptr);
    EXPECT_EQ(link_atom->type, "ListLink");

    // Check the link has the correct targets
    const Link* link = dynamic_cast<const Link*>(link_atom);
    ASSERT_NE(link, nullptr);
    ASSERT_EQ(link->targets.size(), 2);
    EXPECT_FALSE(AtomSpace::is_node(*link_atom));
    EXPECT_TRUE(AtomSpace::is_link(*link_atom));
}

// Test atom_from_document with node document
TEST_F(AtomSpaceTest, AtomFromNodeDocument) {
    // Create a node document
    auto doc = create_node_doc("ConceptNode", "test_node");

    // Create atom from document
    unique_ptr<Atom> atom(space->atom_from_document(doc));

    // Verify the atom is correct
    ASSERT_NE(atom, nullptr);
    EXPECT_EQ(atom->type, "ConceptNode");

    Node* node = dynamic_cast<Node*>(atom.get());
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->name, "test_node");
}

// Test atom_from_document with link document
TEST_F(AtomSpaceTest, AtomFromLinkDocument) {
    // First create some nodes and get their handles
    unique_ptr<char> node1_handle(space->add_node("ConceptNode", "node1"));
    unique_ptr<char> node2_handle(space->add_node("ConceptNode", "node2"));

    // Create a link document referencing these nodes
    auto doc = create_link_doc("ListLink", {node1_handle.get(), node2_handle.get()});

    // Create atom from document
    unique_ptr<Atom> atom(space->atom_from_document(doc));

    // Verify the atom is correct
    ASSERT_NE(atom, nullptr);
    EXPECT_EQ(atom->type, "ListLink");

    Link* link = dynamic_cast<Link*>(atom.get());
    ASSERT_NE(link, nullptr);
    ASSERT_EQ(link->targets.size(), 2);
}

TEST_F(AtomSpaceTest, CommitNodesLocalOnly) {
    // Add nodes to local trie
    unique_ptr<char> handle1(space->add_node("ConceptNode", "node1"));
    unique_ptr<char> handle2(space->add_node("ConceptNode", "node2"));

    // Commit changes to DB
    space->commit_changes(AtomSpace::LOCAL_ONLY);

    // Verify DB calls
    ASSERT_EQ(mock_db->add_node_calls.size(), 0);
}

// Test commit_changes for links
TEST_F(AtomSpaceTest, CommitLinksLocalOnly) {
    // Add nodes and a link
    unique_ptr<char> node1_handle(space->add_node("ConceptNode", "node1"));
    unique_ptr<char> node2_handle(space->add_node("ConceptNode", "node2"));

    vector<string> targets;
    targets.push_back(node1_handle.get());
    targets.push_back(node2_handle.get());

    unique_ptr<char> link_handle(space->add_link("ListLink", targets));

    // Commit changes to DB
    space->commit_changes(AtomSpace::LOCAL_ONLY);

    // Verify DB calls for nodes and link
    ASSERT_EQ(mock_db->add_node_calls.size(), 0);
    ASSERT_EQ(mock_db->add_link_calls.size(), 0);
}

TEST_F(AtomSpaceTest, CommitNodesRemote) {
    // Add nodes to local trie
    space->add_node("ConceptNode", "node1");
    space->add_node("ConceptNode", "node2");

    // Commit changes to DB
    space->commit_changes(AtomSpace::REMOTE_ONLY);

    // Verify DB calls
    ASSERT_EQ(mock_db->add_node_calls.size(), 2);
    EXPECT_EQ(get<0>(mock_db->add_node_calls[0]), "ConceptNode");
    EXPECT_EQ(get<0>(mock_db->add_node_calls[1]), "ConceptNode");

    // HandleTrie.traverse() does not guarantee order, so we sort the names
    vector<string> called_nodes_names;
    called_nodes_names.push_back(get<1>(mock_db->add_node_calls[0]));
    called_nodes_names.push_back(get<1>(mock_db->add_node_calls[1]));
    sort(called_nodes_names.begin(), called_nodes_names.end());
    EXPECT_EQ(called_nodes_names[0], "node1");
    EXPECT_EQ(called_nodes_names[1], "node2");
}

TEST_F(AtomSpaceTest, CommitLinksRemote) {
    // Add nodes and a link
    unique_ptr<char> node1_handle(space->add_node("ConceptNode", "node1"));
    unique_ptr<char> node2_handle(space->add_node("ConceptNode", "node2"));

    vector<string> targets;
    targets.push_back(node1_handle.get());
    targets.push_back(node2_handle.get());

    unique_ptr<char> link_handle(space->add_link("ListLink", targets));

    // Commit changes to DB
    space->commit_changes(AtomSpace::REMOTE_ONLY);

    ASSERT_EQ(mock_db->add_link_calls.size(), 1);
    EXPECT_EQ(get<0>(mock_db->add_link_calls[0]), "ListLink");
    EXPECT_EQ(get<1>(mock_db->add_link_calls[0]).size(), targets.size());
    for (size_t i = 0; i < targets.size(); ++i) {
        EXPECT_EQ(get<1>(mock_db->add_link_calls[0])[i], targets[i]);
    }
}

// Global test environment for AtomDB initialization
class AtomDBEnvironment : public ::testing::Environment {
   public:
    void SetUp() override {
        // Set environment variables needed for initialization
        setenv("DAS_REDIS_HOSTNAME", "localhost", 1);
        setenv("DAS_REDIS_PORT", "29000", 1);
        setenv("DAS_USE_REDIS_CLUSTER", "false", 1);
        setenv("DAS_MONGODB_HOSTNAME", "localhost", 1);
        setenv("DAS_MONGODB_PORT", "28000", 1);
        setenv("DAS_MONGODB_USERNAME", "dbadmin", 1);
        setenv("DAS_MONGODB_PASSWORD", "dassecret", 1);

        // Initialize the singleton once
        AtomDBSingleton::init();
        ServiceBusSingleton::init("localhost:31701", "localhost:31702", 31700, 31799);
    }

    void TearDown() override {
        // Any cleanup if needed
    }
};

// In your main function or at global scope
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    // Register the environment for setup
    ::testing::AddGlobalTestEnvironment(new AtomDBEnvironment);
    return RUN_ALL_TESTS();
}
