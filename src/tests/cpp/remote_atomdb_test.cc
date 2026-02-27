#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <vector>

#include "InMemoryDB.h"
#include "Link.h"
#include "LinkSchema.h"
#include "Node.h"
#include "RemoteAtomDB.h"
#include "RemoteAtomDBPeer.h"

using namespace atomdb;
using namespace atoms;
using namespace std;

// =============================================================================
// RemoteAtomDBPeer tests - peer with InMemoryDB as both remote and local
// =============================================================================

class RemoteAtomDBPeerTest : public ::testing::Test {
   protected:
    void SetUp() override {
        remote_ = make_shared<InMemoryDB>("peer_remote_");
        local_ = make_shared<InMemoryDB>("peer_local_");
        peer_ = make_shared<RemoteAtomDBPeer>(remote_, local_, "test_peer");
    }

    void TearDown() override {}

    shared_ptr<InMemoryDB> remote_;
    shared_ptr<InMemoryDB> local_;
    shared_ptr<RemoteAtomDBPeer> peer_;
};

TEST_F(RemoteAtomDBPeerTest, AddAndGetNodes) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");

    string human_handle = peer_->add_node(human, false);
    string monkey_handle = peer_->add_node(monkey, false);

    EXPECT_FALSE(human_handle.empty());
    EXPECT_FALSE(monkey_handle.empty());
    EXPECT_TRUE(peer_->node_exists(human_handle));
    EXPECT_TRUE(peer_->node_exists(monkey_handle));

    auto retrieved = peer_->get_node(human_handle);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->handle(), human_handle);
}

TEST_F(RemoteAtomDBPeerTest, AddAndGetLinks) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto similarity = new Node("Symbol", "Similarity");

    string human_handle = peer_->add_node(human, false);
    string monkey_handle = peer_->add_node(monkey, false);
    string similarity_handle = peer_->add_node(similarity, false);

    auto link = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link_handle = peer_->add_link(link, false);

    EXPECT_FALSE(link_handle.empty());
    EXPECT_TRUE(peer_->link_exists(link_handle));

    auto retrieved = peer_->get_link(link_handle);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->handle(), link_handle);
}

TEST_F(RemoteAtomDBPeerTest, GetFromCacheThenRemote) {
    // Add to remote only (bypass peer)
    auto human = new Node("Symbol", "\"human\"");
    string human_handle = remote_->add_node(human, false);

    // Peer should find it via remote
    auto retrieved = peer_->get_node(human_handle);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->handle(), human_handle);

    // Second get should come from cache
    auto cached = peer_->get_node(human_handle);
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(cached->handle(), human_handle);
}

TEST_F(RemoteAtomDBPeerTest, PersistsToLocal) {
    auto human = new Node("Symbol", "\"human\"");
    string human_handle = peer_->add_node(human, false);

    // cache should have it
    EXPECT_TRUE(peer_->node_exists(human_handle));
    auto from_cache = peer_->get_node(human_handle);
    ASSERT_NE(from_cache, nullptr);
    EXPECT_EQ(from_cache->handle(), human_handle);
}

TEST_F(RemoteAtomDBPeerTest, QueryForPattern) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto mammal = new Node("Symbol", "\"mammal\"");
    auto inheritance = new Node("Symbol", "Inheritance");

    string human_handle = peer_->add_node(human, false);
    string monkey_handle = peer_->add_node(monkey, false);
    string mammal_handle = peer_->add_node(mammal, false);
    string inheritance_handle = peer_->add_node(inheritance, false);

    auto link1 = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});
    auto link2 = new Link("Expression", {inheritance_handle, monkey_handle, mammal_handle});
    string link1_handle = peer_->add_link(link1, false);
    string link2_handle = peer_->add_link(link2, false);

    peer_->re_index_patterns(true);

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

    auto result = peer_->query_for_pattern(link_schema);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->size(), 2);

    vector<string> handles;
    auto it = result->get_iterator();
    char* h;
    while ((h = it->next()) != nullptr) {
        handles.push_back(h);
    }
    EXPECT_TRUE(find(handles.begin(), handles.end(), link1_handle) != handles.end());
    EXPECT_TRUE(find(handles.begin(), handles.end(), link2_handle) != handles.end());
}

TEST_F(RemoteAtomDBPeerTest, QueryForTargets) {
    auto node1 = new Node("Symbol", "Node1");
    auto node2 = new Node("Symbol", "Node2");
    auto similarity = new Node("Symbol", "Similarity");

    string node1_handle = peer_->add_node(node1, false);
    string node2_handle = peer_->add_node(node2, false);
    string similarity_handle = peer_->add_node(similarity, false);

    auto link = new Link("Expression", {similarity_handle, node1_handle, node2_handle});
    string link_handle = peer_->add_link(link, false);

    auto targets = peer_->query_for_targets(link_handle);
    ASSERT_NE(targets, nullptr);
    EXPECT_EQ(targets->size(), 3);
    EXPECT_EQ(string(targets->get_handle(0)), similarity_handle);
    EXPECT_EQ(string(targets->get_handle(1)), node1_handle);
    EXPECT_EQ(string(targets->get_handle(2)), node2_handle);
}

TEST_F(RemoteAtomDBPeerTest, QueryForIncomingSet) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto similarity = new Node("Symbol", "Similarity");

    string human_handle = peer_->add_node(human, false);
    string monkey_handle = peer_->add_node(monkey, false);
    string similarity_handle = peer_->add_node(similarity, false);

    auto link = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link_handle = peer_->add_link(link, false);

    auto incoming = peer_->query_for_incoming_set(human_handle);
    ASSERT_NE(incoming, nullptr);
    EXPECT_EQ(incoming->size(), 1);

    auto it = incoming->get_iterator();
    char* h = it->next();
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(string(h), link_handle);
}

TEST_F(RemoteAtomDBPeerTest, DeleteLink) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto similarity = new Node("Symbol", "Similarity");

    string human_handle = peer_->add_node(human, false);
    string monkey_handle = peer_->add_node(monkey, false);
    string similarity_handle = peer_->add_node(similarity, false);

    auto link = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link_handle = peer_->add_link(link, false);

    bool deleted = peer_->delete_link(link_handle, false);
    EXPECT_TRUE(deleted);
    EXPECT_FALSE(peer_->link_exists(link_handle));
}

TEST_F(RemoteAtomDBPeerTest, GetUid) { EXPECT_EQ(peer_->get_uid(), "test_peer"); }

TEST_F(RemoteAtomDBPeerTest, AllowNestedIndexing) { EXPECT_FALSE(peer_->allow_nested_indexing()); }

TEST_F(RemoteAtomDBPeerTest, FetchAndRelease) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto mammal = new Node("Symbol", "\"mammal\"");
    auto inheritance = new Node("Symbol", "Inheritance");

    // Add nodes and links to the remote DB directly (bypass peer)
    string human_handle = remote_->add_node(human, false);
    string monkey_handle = remote_->add_node(monkey, false);
    string mammal_handle = remote_->add_node(mammal, false);
    string inheritance_handle = remote_->add_node(inheritance, false);

    auto link1 = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});
    auto link2 = new Link("Expression", {inheritance_handle, monkey_handle, mammal_handle});
    string link1_handle = remote_->add_link(link1, false);
    string link2_handle = remote_->add_link(link2, false);
    remote_->re_index_patterns(true);

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

    // fetch() should pull the pattern results into the cache
    peer_->fetch(link_schema);

    // The peer should now answer the query from its cache
    auto result = peer_->query_for_pattern(link_schema);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->size(), 2);

    // Atoms from the query results should be in the cache (retrievable via peer)
    auto atom1 = peer_->get_atom(link1_handle);
    ASSERT_NE(atom1, nullptr);
    EXPECT_EQ(atom1->handle(), link1_handle);

    // release() should move cached atoms to local_persistence and remove from cache
    peer_->release(link_schema);

    // After release, atoms should be in local_persistence
    EXPECT_TRUE(local_->atom_exists(link1_handle));
    EXPECT_TRUE(local_->atom_exists(link2_handle));

    // The peer should still find the atoms (via local_persistence fallback)
    auto after_release = peer_->get_atom(link1_handle);
    ASSERT_NE(after_release, nullptr);
    EXPECT_EQ(after_release->handle(), link1_handle);
}

TEST_F(RemoteAtomDBPeerTest, ReleaseWithoutLocalPersistence) {
    // Create a peer without local persistence (nullptr)
    auto remote = make_shared<InMemoryDB>("no_local_remote_");
    auto peer_no_local = make_shared<RemoteAtomDBPeer>(remote, nullptr, "no_local_peer");

    auto human = new Node("Symbol", "\"human\"");
    auto mammal = new Node("Symbol", "\"mammal\"");
    auto inheritance = new Node("Symbol", "Inheritance");

    string human_handle = remote->add_node(human, false);
    string mammal_handle = remote->add_node(mammal, false);
    string inheritance_handle = remote->add_node(inheritance, false);

    auto link = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});
    string link_handle = remote->add_link(link, false);
    remote->re_index_patterns(true);

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

    peer_no_local->fetch(link_schema);
    auto result = peer_no_local->query_for_pattern(link_schema);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->size(), 1);

    // release() should not crash when there's no local persistence
    peer_no_local->release(link_schema);

    // Atom should still be available via the remote fallback
    auto atom = peer_no_local->get_atom(link_handle);
    ASSERT_NE(atom, nullptr);
}

// Resolves path to config file from Bazel runfiles or workspace.
string resolve_config_path(const string& filename) {
    auto exists = [](const string& path) {
        ifstream f(path);
        return f.good();
    };

    const char* srcdir = std::getenv("TEST_SRCDIR");
    const char* workspace = std::getenv("TEST_WORKSPACE");
    if (srcdir) {
        if (workspace) {
            string path = string(srcdir) + "/" + string(workspace) + "/tests/assets/" + filename;
            if (exists(path)) return path;
        }
        for (const auto& prefix : {"", "src/", "das/", "_main/"}) {
            string path = string(srcdir) + "/" + prefix + "tests/assets/" + filename;
            if (exists(path)) return path;
        }
    }
    if (exists("tests/assets/" + filename)) return "tests/assets/" + filename;
    if (exists("src/tests/assets/" + filename)) return "src/tests/assets/" + filename;
    return "";
}

// =============================================================================
// RemoteAtomDB tests - uses tests/assets/remotedb_config.json
// =============================================================================

class RemoteAtomDBTest : public ::testing::Test {
   protected:
    void SetUp() override {
        config_path_ = resolve_config_path("remotedb_config.json");
        ASSERT_FALSE(config_path_.empty())
            << "Could not find tests/assets/remotedb_config.json (TEST_SRCDIR="
            << (std::getenv("TEST_SRCDIR") ? std::getenv("TEST_SRCDIR") : "unset") << ")";
        db_ = make_shared<RemoteAtomDB>(config_path_);
    }

    void TearDown() override {}

    string config_path_;
    shared_ptr<RemoteAtomDB> db_;
};

TEST_F(RemoteAtomDBTest, Initialization) {
    const auto& peers = db_->get_remote_dbs();
    EXPECT_EQ(peers.size(), 2u);
    EXPECT_NE(peers.find("peer1"), peers.end());
    EXPECT_NE(peers.find("peer2"), peers.end());
}

TEST_F(RemoteAtomDBTest, GetPeer) {
    auto* peer1 = db_->get_peer("peer1");
    auto* peer2 = db_->get_peer("peer2");
    ASSERT_NE(peer1, nullptr);
    ASSERT_NE(peer2, nullptr);
    EXPECT_EQ(peer1->get_uid(), "peer1");
    EXPECT_EQ(peer2->get_uid(), "peer2");

    EXPECT_EQ(db_->get_peer("nonexistent"), nullptr);
}

TEST_F(RemoteAtomDBTest, AddAndGetAcrossPeers) {
    auto human = new Node("Symbol", "\"human\"");
    string human_handle = db_->add_node(human, false);

    EXPECT_FALSE(human_handle.empty());
    EXPECT_TRUE(db_->node_exists(human_handle));

    auto retrieved = db_->get_node(human_handle);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->handle(), human_handle);
}

TEST_F(RemoteAtomDBTest, AddLinksAndQuery) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto mammal = new Node("Symbol", "\"mammal\"");
    auto inheritance = new Node("Symbol", "Inheritance");

    string human_handle = db_->add_node(human, false);
    string monkey_handle = db_->add_node(monkey, false);
    string mammal_handle = db_->add_node(mammal, false);
    string inheritance_handle = db_->add_node(inheritance, false);

    auto link1 = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});
    auto link2 = new Link("Expression", {inheritance_handle, monkey_handle, mammal_handle});
    string link1_handle = db_->add_link(link1, false);
    string link2_handle = db_->add_link(link2, false);

    db_->re_index_patterns(true);

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

    auto result = db_->query_for_pattern(link_schema);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->size(), 2);
}

TEST_F(RemoteAtomDBTest, DeleteOperations) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto similarity = new Node("Symbol", "Similarity");

    string human_handle = db_->add_node(human, false);
    string monkey_handle = db_->add_node(monkey, false);
    string similarity_handle = db_->add_node(similarity, false);

    auto link = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link_handle = db_->add_link(link, false);

    bool deleted = db_->delete_link(link_handle, false);
    EXPECT_TRUE(deleted);
    EXPECT_FALSE(db_->link_exists(link_handle));
}

// =============================================================================
// RemoteAtomDB config tests - config without local_persistence
// Uses tests/assets/remotedb_config_single.json
// =============================================================================

class RemoteAtomDBConfigTest : public ::testing::Test {
   protected:
    void SetUp() override {
        config_path_ = resolve_config_path("remotedb_config_single.json");
        ASSERT_FALSE(config_path_.empty()) << "Could not find tests/assets/remotedb_config_single.json";
        db_ = make_shared<RemoteAtomDB>(config_path_);
    }

    void TearDown() override {}

    string config_path_;
    shared_ptr<RemoteAtomDB> db_;
};

TEST_F(RemoteAtomDBConfigTest, SingleConfigWorks) {
    const auto& peers = db_->get_remote_dbs();
    EXPECT_EQ(peers.size(), 1u);
    EXPECT_NE(peers.find("single_peer"), peers.end());

    auto human = new Node("Symbol", "\"human\"");
    string human_handle = db_->add_node(human, false);
    EXPECT_TRUE(db_->node_exists(human_handle));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
