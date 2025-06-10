#include <gtest/gtest.h>

#include <cstdlib>

#include "AtomDBAPITypes.h"
#include "AtomDBSingleton.h"
#include "AtomSpace.h"
#include "AtomSpaceTypes.h"
#include "ServiceBusSingleton.h"

using namespace std;
using namespace atomspace;
using namespace atomdb;
using namespace atomdb_api_types;

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
    vector<tuple<string, string, CustomAttributesMap>> add_node_calls;
    vector<tuple<string, vector<string>, CustomAttributesMap>> add_link_calls;

    // Mock documents to return
    map<string, shared_ptr<AtomDocument>> docs;

    // AtomDB interface implementation
    shared_ptr<HandleSet> query_for_pattern(shared_ptr<char>) override { return nullptr; }
    shared_ptr<HandleList> query_for_targets(shared_ptr<char>) override { return nullptr; }
    shared_ptr<HandleList> query_for_targets(char*) override { return nullptr; }

    shared_ptr<AtomDocument> get_atom_document(const char* handle) override {
        get_atom_calls.push_back(handle);
        auto it = docs.find(handle);
        if (it != docs.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool link_exists(const char*) override { return false; }
    vector<string> links_exist(const vector<string>&) override { return {}; }

    char* add_node(const char* type, const char* name, const CustomAttributesMap& attrs) override {
        add_node_calls.push_back({type, name, attrs});
        string handle = string("node_") + type + "_" + name;
        return strdup(handle.c_str());
    }

    char* add_link(const char* type,
                   char** targets,
                   size_t targets_size,
                   const CustomAttributesMap& attrs) override {
        vector<string> target_handles;
        for (size_t i = 0; i < targets_size; i++) {
            target_handles.push_back(targets[i]);
        }
        add_link_calls.push_back({type, target_handles, attrs});
        return strdup("link_handle");
    }

    void attention_broker_setup() override {}
};

class MockAtomSpace : public AtomSpace {
   public:
    MockAtomSpace() : AtomSpace() { this->db = make_shared<MockAtomDB>(); }
};

// Fixture for AtomSpace tests
class AtomSpaceTest : public ::testing::Test {
   protected:
    shared_ptr<MockAtomDB> mock_db;
    AtomSpace* space;

    void SetUp() override {
        mock_db = make_shared<MockAtomDB>();
        space = new MockAtomSpace();
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
    char* handle = space->add_node("ConceptNode", "test");

    // Retrieve it with LOCAL_ONLY scope
    const Atom* atom = space->get_atom(handle, AtomSpace::LOCAL_ONLY);

    // Verify we got it back
    ASSERT_NE(atom, nullptr);
    EXPECT_EQ(atom->type, "ConceptNode");
    EXPECT_EQ(dynamic_cast<const Node*>(atom)->name, "test");

    // Verify we didn't call the DB
    EXPECT_TRUE(mock_db->get_atom_calls.empty());

    free(handle);
}

// Test get_atom for remote retrieval
TEST_F(AtomSpaceTest, GetAtomRemoteOnly) {
    // Setup mock DB to return a document
    auto doc = create_node_doc("ConceptNode", "remote_test");
    mock_db->docs["test_handle"] = doc;

    // Retrieve with REMOTE_ONLY scope
    const Atom* atom = space->get_atom("test_handle", AtomSpace::REMOTE_ONLY);

    // Verify we got the atom from DB
    ASSERT_NE(atom, NULL);
    EXPECT_EQ(atom->type, "ConceptNode");
    EXPECT_EQ(dynamic_cast<const Node*>(atom)->name, "remote_test");

    // Verify we called the DB
    ASSERT_EQ(mock_db->get_atom_calls.size(), 1);
    EXPECT_EQ(mock_db->get_atom_calls[0], "test_handle");
}

// // Test get_atom for local and remote retrieval
// TEST_F(AtomSpaceTest, GetAtomLocalAndRemote) {
//     // Add a node to the local trie
//     char* handle = space->add_node("ConceptNode", "local_test");

//     // Setup mock DB to return a document
//     auto doc = create_node_doc("ConceptNode", "remote_test");
//     mock_db->docs["remote_handle"] = doc;

//     // Retrieve local node with LOCAL_AND_REMOTE scope
//     const Atom* local_atom = space->get_atom(handle, AtomSpace::LOCAL_AND_REMOTE);

//     // Verify we got it from local without calling DB
//     ASSERT_NE(local_atom, nullptr);
//     EXPECT_EQ(local_atom->type, "ConceptNode");
//     EXPECT_EQ(dynamic_cast<const Node*>(local_atom)->name, "local_test");
//     EXPECT_TRUE(mock_db->get_atom_calls.empty());

//     // Retrieve remote node with LOCAL_AND_REMOTE scope
//     const Atom* remote_atom = space->get_

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
