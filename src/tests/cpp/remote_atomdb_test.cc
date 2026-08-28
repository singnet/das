#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "Assignment.h"
#include "AtomDBFactory.h"
#include "InMemoryAccessPermissionTypes.h"
#include "InMemoryDB.h"
#include "InMemoryDBAPITypes.h"
#include "JsonConfig.h"
#include "Link.h"
#include "LinkSchema.h"
#include "Node.h"
#include "ProtectedAtomDB.h"
#include "RemoteAtomDB.h"
#include "RemoteAtomDBPeer.h"

using namespace atomdb;
using namespace atomdb::atomdb_api_types;
using namespace atoms;
using namespace commons;
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

// Builds the Inheritance(x, "mammal") pattern used across several cache tests.
static LinkSchema inheritance_mammal_schema() {
    return LinkSchema({"LINK_TEMPLATE",
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
}

// Populates a backend with two Inheritance(*, "mammal") links and returns their handles.
static vector<string> populate_inheritance_mammal_links(shared_ptr<InMemoryDB> backend) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto mammal = new Node("Symbol", "\"mammal\"");
    auto inheritance = new Node("Symbol", "Inheritance");

    string human_handle = backend->add_node(human);
    string monkey_handle = backend->add_node(monkey);
    string mammal_handle = backend->add_node(mammal);
    string inheritance_handle = backend->add_node(inheritance);

    auto link1 = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});
    auto link2 = new Link("Expression", {inheritance_handle, monkey_handle, mammal_handle});
    string link1_handle = backend->add_link(link1);
    string link2_handle = backend->add_link(link2);
    backend->re_index_patterns(true);
    return {link1_handle, link2_handle};
}

TEST_F(RemoteAtomDBPeerTest, AddAndGetNodes) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");

    string human_handle = peer_->add_node(human);
    string monkey_handle = peer_->add_node(monkey);

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

    string human_handle = peer_->add_node(human);
    string monkey_handle = peer_->add_node(monkey);
    string similarity_handle = peer_->add_node(similarity);

    auto link = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link_handle = peer_->add_link(link);

    EXPECT_FALSE(link_handle.empty());
    EXPECT_TRUE(peer_->link_exists(link_handle));

    auto retrieved = peer_->get_link(link_handle);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->handle(), link_handle);
}

TEST_F(RemoteAtomDBPeerTest, GetFromCacheThenRemote) {
    // Add to remote only (bypass peer)
    auto human = new Node("Symbol", "\"human\"");
    string human_handle = remote_->add_node(human);

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
    auto monkey = new Node("Symbol", "\"monkey\"");
    string human_handle = peer_->add_node(human);
    string monkey_handle = peer_->add_node(monkey);

    peer_->release_cache(true, true);

    EXPECT_TRUE(local_->node_exists(human_handle));
    EXPECT_EQ(peer_->get_cached_atom(human_handle), nullptr);

    EXPECT_TRUE(local_->node_exists(monkey_handle));
    EXPECT_EQ(peer_->get_cached_atom(monkey_handle), nullptr);
}

TEST_F(RemoteAtomDBPeerTest, QueryForPattern) {
    // Add atoms to the remote DB directly so that the "not yet fetched" path
    // (which only queries atomdb_) finds them.
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto mammal = new Node("Symbol", "\"mammal\"");
    auto inheritance = new Node("Symbol", "Inheritance");

    string human_handle = remote_->add_node(human);
    string monkey_handle = remote_->add_node(monkey);
    string mammal_handle = remote_->add_node(mammal);
    string inheritance_handle = remote_->add_node(inheritance);

    auto link1 = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});
    auto link2 = new Link("Expression", {inheritance_handle, monkey_handle, mammal_handle});
    string link1_handle = remote_->add_link(link1);
    string link2_handle = remote_->add_link(link2);

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

    string node1_handle = peer_->add_node(node1);
    string node2_handle = peer_->add_node(node2);
    string similarity_handle = peer_->add_node(similarity);

    auto link = new Link("Expression", {similarity_handle, node1_handle, node2_handle});
    string link_handle = peer_->add_link(link);

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

    string human_handle = peer_->add_node(human);
    string monkey_handle = peer_->add_node(monkey);
    string similarity_handle = peer_->add_node(similarity);

    auto link = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link_handle = peer_->add_link(link);

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

    string human_handle = peer_->add_node(human);
    string monkey_handle = peer_->add_node(monkey);
    string similarity_handle = peer_->add_node(similarity);

    auto link = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link_handle = peer_->add_link(link);

    bool deleted = peer_->delete_link(link_handle, false);
    EXPECT_TRUE(deleted);
    EXPECT_FALSE(peer_->link_exists(link_handle));
}

TEST_F(RemoteAtomDBPeerTest, GetUid) { EXPECT_EQ(peer_->get_uid(), "test_peer"); }

TEST_F(RemoteAtomDBPeerTest, AllowNestedIndexing) { EXPECT_FALSE(peer_->allow_nested_indexing()); }

TEST_F(RemoteAtomDBPeerTest, FetchAndRelease) {
    auto handles = populate_inheritance_mammal_links(remote_);
    string link1_handle = handles[0];
    string link2_handle = handles[1];
    LinkSchema link_schema = inheritance_mammal_schema();

    peer_->fetch(link_schema);

    auto result = peer_->query_for_pattern(link_schema);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->size(), 2);
    EXPECT_NE(peer_->get_cached_atom(link1_handle), nullptr);

    peer_->release(link_schema);

    // Fetch only warms the disposable read_cache; release drops it without copying into
    // local_persistence. Atoms remain reachable via the remote backend.
    EXPECT_FALSE(local_->atom_exists(link1_handle));
    EXPECT_FALSE(local_->atom_exists(link2_handle));
    EXPECT_EQ(peer_->get_cached_atom(link1_handle), nullptr);

    auto after_release = peer_->get_atom(link1_handle);
    ASSERT_NE(after_release, nullptr);
    EXPECT_EQ(after_release->handle(), link1_handle);
}

TEST_F(RemoteAtomDBPeerTest, AddAfterFetchRefetchesPatternFromRemote) {
    auto handles = populate_inheritance_mammal_links(remote_);
    string link1_handle = handles[0];
    string link2_handle = handles[1];
    LinkSchema link_schema = inheritance_mammal_schema();

    peer_->fetch(link_schema);
    ASSERT_EQ(peer_->query_for_pattern(link_schema)->size(), 2);
    EXPECT_NE(peer_->get_cached_atom(link1_handle), nullptr);

    // Any write invalidates the fetched-template registry but keeps read-cached atoms in cache.
    auto staged = new Node("Symbol", "\"staged\"");
    string staged_handle = peer_->add_node(staged);
    EXPECT_NE(peer_->get_cached_atom(link1_handle), nullptr);
    EXPECT_NE(peer_->get_cached_atom(link2_handle), nullptr);
    EXPECT_NE(peer_->get_cached_atom(staged_handle), nullptr);

    // Remote gains a new matching link after the template registry was invalidated.
    auto link1 = peer_->get_link(link1_handle);
    ASSERT_NE(link1, nullptr);
    string inheritance_handle = link1->targets[0];
    string mammal_handle = link1->targets[2];

    auto chimp = new Node("Symbol", "\"chimp\"");
    string chimp_handle = remote_->add_node(chimp);
    auto link3 = new Link("Expression", {inheritance_handle, chimp_handle, mammal_handle});
    string link3_handle = remote_->add_link(link3);
    remote_->re_index_patterns(true);

    auto result = peer_->query_for_pattern(link_schema);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->size(), 3);

    vector<string> result_handles;
    auto it = result->get_iterator();
    char* h;
    while ((h = it->next()) != nullptr) {
        result_handles.push_back(h);
    }
    EXPECT_TRUE(find(result_handles.begin(), result_handles.end(), link3_handle) !=
                result_handles.end());
}

TEST_F(RemoteAtomDBPeerTest, DeleteAfterFetchReleasesRemoteCache) {
    auto handles = populate_inheritance_mammal_links(remote_);
    string link1_handle = handles[0];
    string link2_handle = handles[1];
    LinkSchema link_schema = inheritance_mammal_schema();

    peer_->fetch(link_schema);
    EXPECT_NE(peer_->get_cached_atom(link1_handle), nullptr);
    EXPECT_NE(peer_->get_cached_atom(link2_handle), nullptr);

    // Delete removes the target from cache and invalidates the fetched-template registry.
    ASSERT_TRUE(peer_->delete_link(link1_handle, false));
    EXPECT_EQ(peer_->get_cached_atom(link1_handle), nullptr);
    EXPECT_NE(peer_->get_cached_atom(link2_handle), nullptr);

    auto link2 = peer_->get_link(link2_handle);
    ASSERT_NE(link2, nullptr);

    auto chimp = new Node("Symbol", "\"chimp\"");
    string chimp_handle = remote_->add_node(chimp);
    auto link3 = new Link("Expression", {link2->targets[0], chimp_handle, link2->targets[2]});
    string link3_handle = remote_->add_link(link3);
    remote_->re_index_patterns(true);

    auto result = peer_->query_for_pattern(link_schema);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->size(), 3);

    vector<string> result_handles;
    auto it = result->get_iterator();
    char* h;
    while ((h = it->next()) != nullptr) {
        result_handles.push_back(h);
    }
    EXPECT_TRUE(find(result_handles.begin(), result_handles.end(), link3_handle) !=
                result_handles.end());
}

TEST_F(RemoteAtomDBPeerTest, ReleaseWithoutLocalPersistence) {
    // Create a peer without local persistence (nullptr)
    auto remote = make_shared<InMemoryDB>("no_local_remote_");
    auto peer_no_local = make_shared<RemoteAtomDBPeer>(remote, nullptr, "no_local_peer");

    auto human = new Node("Symbol", "\"human\"");
    auto mammal = new Node("Symbol", "\"mammal\"");
    auto inheritance = new Node("Symbol", "Inheritance");

    string human_handle = remote->add_node(human);
    string mammal_handle = remote->add_node(mammal);
    string inheritance_handle = remote->add_node(inheritance);

    auto link = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});
    string link_handle = remote->add_link(link);
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

TEST_F(RemoteAtomDBPeerTest, AtomsCount) {
    EXPECT_EQ(peer_->atom_count(), 0);
    EXPECT_EQ(peer_->empty(), true);

    auto node1 = new Node("Symbol", "Node1");
    auto node2 = new Node("Symbol", "Node2");
    auto similarity = new Node("Symbol", "Similarity");

    peer_->add_node(node1);
    peer_->add_node(node2);
    peer_->add_node(similarity);

    EXPECT_EQ(peer_->atom_count(), 3);

    auto link1 = new Link("Expression", {similarity->handle(), node1->handle(), node2->handle()});
    peer_->add_link(link1);

    EXPECT_EQ(peer_->atom_count(), 4);
    EXPECT_EQ(peer_->empty(), false);

    // Read-cache warming must not inflate the count: the read cache is a view of atoms
    // that already live elsewhere (here: the remote backend).
    auto remote_only = new Node("Symbol", "\"remote_only\"");
    string remote_only_handle = remote_->add_node(remote_only);
    ASSERT_NE(peer_->get_atom(remote_only_handle), nullptr);
    ASSERT_NE(peer_->get_cached_atom(remote_only_handle), nullptr);
    EXPECT_EQ(peer_->atom_count(), 4);
}

// =============================================================================
// RemoteAtomDBPeer readonly / persistence tests
// =============================================================================

TEST(RemoteAtomDBPeerReadonlyTest, RepeatQuerySeesRemoteAfterRelease) {
    auto remote = make_shared<InMemoryDB>("readonly_remote_");
    auto peer = make_shared<RemoteAtomDBPeer>(remote, nullptr, "readonly_peer");

    auto handles = populate_inheritance_mammal_links(remote);
    string link1_handle = handles[0];
    LinkSchema link_schema = inheritance_mammal_schema();

    auto result1 = peer->query_for_pattern(link_schema);
    ASSERT_NE(result1, nullptr);
    EXPECT_EQ(result1->size(), 2u);

    auto link1 = peer->get_link(link1_handle);
    ASSERT_NE(link1, nullptr);
    string inheritance_handle = link1->targets[0];
    string mammal_handle = link1->targets[2];

    auto chimp = new Node("Symbol", "\"chimp\"");
    string chimp_handle = remote->add_node(chimp);
    auto link3 = new Link("Expression", {inheritance_handle, chimp_handle, mammal_handle});
    string link3_handle = remote->add_link(link3);
    remote->re_index_patterns(true);

    // Drop the fetched-schema marker so the next query re-hits the remote backend.
    peer->release(link_schema, false);

    auto result2 = peer->query_for_pattern(link_schema);
    ASSERT_NE(result2, nullptr);
    EXPECT_EQ(result2->size(), 3u);

    vector<string> result_handles;
    auto it = result2->get_iterator();
    char* h;
    while ((h = it->next()) != nullptr) {
        result_handles.push_back(h);
    }
    EXPECT_TRUE(find(result_handles.begin(), result_handles.end(), link3_handle) !=
                result_handles.end());
}

TEST(RemoteAtomDBPeerReadonlyTest, AddStagesThenPersistsOnRelease) {
    auto remote = make_shared<InMemoryDB>("stage_remote_");
    auto local = make_shared<InMemoryDB>("stage_local_");
    auto peer = make_shared<RemoteAtomDBPeer>(remote, local, "stage_peer");

    auto human = new Node("Symbol", "\"human\"");
    string human_handle = peer->add_node(human);

    EXPECT_FALSE(human_handle.empty());
    EXPECT_NE(peer->get_cached_atom(human_handle), nullptr);
    EXPECT_FALSE(local->node_exists(human_handle));

    peer->release_cache(true, false);
    EXPECT_TRUE(local->node_exists(human_handle));
    EXPECT_TRUE(peer->node_exists(human_handle));
}

TEST(RemoteAtomDBPeerReadonlyTest, AddFailsWithoutLocalPersistence) {
    auto remote = make_shared<InMemoryDB>("read_only_remote_");
    auto peer = make_shared<RemoteAtomDBPeer>(remote, nullptr, "read_only_peer");

    auto human = new Node("Symbol", "\"human\"");
    EXPECT_TRUE(peer->add_node(human).empty());
    EXPECT_TRUE(peer->add_link(new Link("Expression", {"a", "b", "c"})).empty());
}

TEST(RemoteAtomDBPeerReadonlyTest, DeleteAtomFromLocalPersistence) {
    auto remote = make_shared<InMemoryDB>("delete_remote_");
    auto local = make_shared<InMemoryDB>("delete_local_");
    auto peer = make_shared<RemoteAtomDBPeer>(remote, local, "delete_peer");

    auto human = new Node("Symbol", "\"human\"");
    string human_handle = local->add_node(human);

    EXPECT_TRUE(peer->delete_atom(human_handle, false));
    EXPECT_FALSE(peer->atom_exists(human_handle));
    EXPECT_FALSE(local->atom_exists(human_handle));
}

TEST(RemoteAtomDBPeerReadonlyTest, FetchWarmsCache) {
    auto remote = make_shared<InMemoryDB>("fetch_remote_");
    auto peer = make_shared<RemoteAtomDBPeer>(remote, nullptr, "fetch_peer");
    auto handles = populate_inheritance_mammal_links(remote);
    LinkSchema link_schema = inheritance_mammal_schema();

    peer->fetch(link_schema);
    EXPECT_NE(peer->get_cached_atom(handles[0]), nullptr);

    auto result = peer->query_for_pattern(link_schema);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->size(), 2u);
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

JsonConfig load_config(const string& filename) {
    ifstream f(filename);
    if (!f.good()) {
        RAISE_ERROR("RemoteAtomDBTest: Cannot open config file: " + filename);
    }
    stringstream buf;
    buf << f.rdbuf();
    return JsonConfig(nlohmann::json::parse(buf.str()));
}

class RemoteAtomDBTest : public ::testing::Test {
   protected:
    void SetUp() override {
        config_path_ = resolve_config_path("remotedb_config.json");
        ASSERT_FALSE(config_path_.empty())
            << "Could not find tests/assets/remotedb_config.json (TEST_SRCDIR="
            << (std::getenv("TEST_SRCDIR") ? std::getenv("TEST_SRCDIR") : "unset") << ")";
        auto json_config = load_config(config_path_);
        json_config["type"] = "remotedb";
        auto db = AtomDBFactory::create(json_config, "");
        db_ = dynamic_pointer_cast<RemoteAtomDB>(db);
        ASSERT_NE(db_, nullptr);
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
    string human_handle = db_->add_node(human);

    EXPECT_FALSE(human_handle.empty());
    EXPECT_TRUE(db_->node_exists(human_handle));

    auto retrieved = db_->get_node(human_handle);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->handle(), human_handle);
}

TEST_F(RemoteAtomDBTest, AddLinksAndRetrieve) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto mammal = new Node("Symbol", "\"mammal\"");
    auto inheritance = new Node("Symbol", "Inheritance");

    string human_handle = db_->add_node(human);
    string monkey_handle = db_->add_node(monkey);
    string mammal_handle = db_->add_node(mammal);
    string inheritance_handle = db_->add_node(inheritance);

    auto link1 = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});
    auto link2 = new Link("Expression", {inheritance_handle, monkey_handle, mammal_handle});
    string link1_handle = db_->add_link(link1);
    string link2_handle = db_->add_link(link2);

    EXPECT_TRUE(db_->link_exists(link1_handle));
    EXPECT_TRUE(db_->link_exists(link2_handle));

    auto retrieved1 = db_->get_link(link1_handle);
    ASSERT_NE(retrieved1, nullptr);
    EXPECT_EQ(retrieved1->handle(), link1_handle);

    auto retrieved2 = db_->get_link(link2_handle);
    ASSERT_NE(retrieved2, nullptr);
    EXPECT_EQ(retrieved2->handle(), link2_handle);
}

TEST_F(RemoteAtomDBTest, DeleteOperations) {
    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto similarity = new Node("Symbol", "Similarity");

    string human_handle = db_->add_node(human);
    string monkey_handle = db_->add_node(monkey);
    string similarity_handle = db_->add_node(similarity);

    auto link = new Link("Expression", {similarity_handle, human_handle, monkey_handle});
    string link_handle = db_->add_link(link);

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
        auto json_config = load_config(config_path_);
        json_config["type"] = "remotedb";
        auto db = AtomDBFactory::create(json_config, "");
        db_ = dynamic_pointer_cast<RemoteAtomDB>(db);
        ASSERT_NE(db_, nullptr);
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
    string human_handle = db_->add_node(human);
    EXPECT_TRUE(db_->node_exists(human_handle));
}

// =============================================================================
// RemoteAtomDB federation tests (cache-first probing + metadata aggregation)
//
// These exercise the facade with controllable, in-process peers (no live config
// or external connection). A NestedInMemoryDB models a nested-indexing backend
// (e.g. MorkDB) that returns per-handle metadata from query_for_pattern.
// =============================================================================

// In-memory backend that advertises nested indexing and attaches per-handle
// metta expressions / assignments to its pattern query results.
class NestedInMemoryDB : public InMemoryDB {
   public:
    explicit NestedInMemoryDB(const string& context) : InMemoryDB(context) {}

    bool allow_nested_indexing() override { return true; }

    shared_ptr<HandleSet> query_for_pattern(const LinkSchema& link_schema) override {
        auto base = InMemoryDB::query_for_pattern(link_schema);
        auto result = make_shared<HandleSetInMemory>();
        if (base) {
            auto it = base->get_iterator();
            char* h;
            while ((h = it->next()) != nullptr) {
                string handle(h);
                map<string, string> metta = {{"$x", "(nested " + handle.substr(0, 6) + ")"}};
                Assignment assignment;
                assignment.assign("$x", handle);
                result->add_handle(handle, metta, assignment);
            }
        }
        return result;
    }
};

// Local persistence backend that advertises composite_type_enabled.
class CompositeTypeEnabledInMemoryDB : public InMemoryDB {
   public:
    explicit CompositeTypeEnabledInMemoryDB(const string& context) : InMemoryDB(context) {}

    bool composite_type_enabled() const override { return true; }
};

// Backend pointing at a protected database, like a RedisMongoDB whose Mongo config flags it.
class ProtectedInMemoryDB : public InMemoryDB {
   public:
    explicit ProtectedInMemoryDB(const string& context) : InMemoryDB(context) {}

    ProtectionMode get_protection_mode() const override { return ProtectionMode::PROTECTED; }
};

class FullAccessProtectedInMemoryDB : public ProtectedInMemoryDB {
   public:
    explicit FullAccessProtectedInMemoryDB(const string& context, const string& allowed_key)
        : ProtectedInMemoryDB(context), allowed_key(allowed_key) {}

    vector<shared_ptr<AccessPermissionDocument>> get_access_permissions(
        const PublicKey& public_key) const override {
        if (!public_key.is_single_key() || public_key.keys.empty() ||
            public_key.keys[0] != this->allowed_key) {
            return {};
        }
        auto document = make_shared<InMemoryAccessPermissionDocument>();
        document->set_access_key(this->allowed_key);
        document->set_full_access(true);
        return {document};
    }

   private:
    string allowed_key;
};

class ForwardInMemoryDB : public InMemoryDB {
   public:
    explicit ForwardInMemoryDB(const string& context) : InMemoryDB(context) {}

    ProtectionMode get_protection_mode() const override { return ProtectionMode::FORWARD; }
};

TEST(RemoteAtomDBFederationTest, MetadataAggregationFromNestedPeer) {
    auto backend = make_shared<NestedInMemoryDB>("fed_nested_backend_");
    auto handles = populate_inheritance_mammal_links(backend);

    map<string, shared_ptr<RemoteAtomDBPeer>> peers;
    peers["nested"] = make_shared<RemoteAtomDBPeer>(backend, nullptr, "nested");
    auto db = make_shared<RemoteAtomDB>(peers);

    // All peers are nested-indexing -> facade advertises nested indexing.
    EXPECT_TRUE(db->allow_nested_indexing());

    auto result = db->query_for_pattern(inheritance_mammal_schema());
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->size(), 2u);

    // Metadata must survive aggregation for every matched handle.
    auto it = result->get_iterator();
    char* h;
    int checked = 0;
    while ((h = it->next()) != nullptr) {
        string handle(h);
        auto metta = result->get_metta_expressions_by_handle(handle);
        EXPECT_FALSE(metta.empty()) << "metta lost for " << handle;
        EXPECT_EQ(metta["$x"], "(nested " + handle.substr(0, 6) + ")");

        auto assignment = result->get_assignments_by_handle(handle);
        EXPECT_EQ(assignment.get("$x"), handle);
        checked++;
    }
    EXPECT_EQ(checked, 2);
}

TEST(RemoteAtomDBFederationTest, MixedPeersDowngradeAndDeduplicate) {
    // peer1: nested-indexing backend; peer2: plain in-memory backend.
    // Both contain the SAME two links so the facade must dedup by handle.
    auto nested_backend = make_shared<NestedInMemoryDB>("fed_mixed_nested_");
    auto plain_backend = make_shared<InMemoryDB>("fed_mixed_plain_");
    auto nested_handles = populate_inheritance_mammal_links(nested_backend);
    auto plain_handles = populate_inheritance_mammal_links(plain_backend);
    ASSERT_EQ(nested_handles, plain_handles);  // identical content -> identical handles

    map<string, shared_ptr<RemoteAtomDBPeer>> peers;
    peers["nested"] = make_shared<RemoteAtomDBPeer>(nested_backend, nullptr, "nested");
    peers["plain"] = make_shared<RemoteAtomDBPeer>(plain_backend, nullptr, "plain");
    auto db = make_shared<RemoteAtomDB>(peers);

    // Mixed nested/non-nested peers -> facade downgrades to false.
    EXPECT_FALSE(db->allow_nested_indexing());

    auto result = db->query_for_pattern(inheritance_mammal_schema());
    ASSERT_NE(result, nullptr);
    // Same two handles from both peers, deduplicated to a unique count of 2.
    EXPECT_EQ(result->size(), 2u);
}

TEST(RemoteAtomDBFederationTest, CompositeTypeEnabledAggregation) {
    // No peers -> false.
    {
        map<string, shared_ptr<RemoteAtomDBPeer>> peers;
        auto db = make_shared<RemoteAtomDB>(peers);
        EXPECT_FALSE(db->composite_type_enabled());
    }

    // All disabled (no local persistence, or local persistence with flag off) -> false.
    {
        auto remote1 = make_shared<InMemoryDB>("cte_all_off_remote1_");
        auto remote2 = make_shared<InMemoryDB>("cte_all_off_remote2_");
        auto local_off = make_shared<InMemoryDB>("cte_all_off_local_");
        map<string, shared_ptr<RemoteAtomDBPeer>> peers;
        peers["peer1"] = make_shared<RemoteAtomDBPeer>(remote1, nullptr, "peer1");
        peers["peer2"] = make_shared<RemoteAtomDBPeer>(remote2, local_off, "peer2");
        auto db = make_shared<RemoteAtomDB>(peers);
        EXPECT_FALSE(db->composite_type_enabled());
    }

    // Mixed: one enabled + one disabled -> true.
    {
        auto remote1 = make_shared<InMemoryDB>("cte_mixed_remote1_");
        auto remote2 = make_shared<InMemoryDB>("cte_mixed_remote2_");
        auto local_on = make_shared<CompositeTypeEnabledInMemoryDB>("cte_mixed_local_on_");
        auto local_off = make_shared<InMemoryDB>("cte_mixed_local_off_");
        map<string, shared_ptr<RemoteAtomDBPeer>> peers;
        peers["enabled"] = make_shared<RemoteAtomDBPeer>(remote1, local_on, "enabled");
        peers["disabled"] = make_shared<RemoteAtomDBPeer>(remote2, local_off, "disabled");
        auto db = make_shared<RemoteAtomDB>(peers);
        EXPECT_TRUE(db->composite_type_enabled());
    }

    // All enabled -> true.
    {
        auto remote1 = make_shared<InMemoryDB>("cte_all_on_remote1_");
        auto remote2 = make_shared<InMemoryDB>("cte_all_on_remote2_");
        auto local1 = make_shared<CompositeTypeEnabledInMemoryDB>("cte_all_on_local1_");
        auto local2 = make_shared<CompositeTypeEnabledInMemoryDB>("cte_all_on_local2_");
        map<string, shared_ptr<RemoteAtomDBPeer>> peers;
        peers["peer1"] = make_shared<RemoteAtomDBPeer>(remote1, local1, "peer1");
        peers["peer2"] = make_shared<RemoteAtomDBPeer>(remote2, local2, "peer2");
        auto db = make_shared<RemoteAtomDB>(peers);
        EXPECT_TRUE(db->composite_type_enabled());
    }
}

TEST(RemoteAtomDBFederationTest, PeerIsProtectedFollowsRemoteBackend) {
    // Neither the remote backend nor the local persistence is protected.
    {
        auto remote = make_shared<InMemoryDB>("prot_none_remote_");
        auto local = make_shared<InMemoryDB>("prot_none_local_");
        auto peer = make_shared<RemoteAtomDBPeer>(remote, local, "peer");
        EXPECT_EQ(peer->get_protection_mode(), ProtectionMode::UNPROTECTED);
    }

    // Read-only peer (no local persistence) over an unprotected remote.
    {
        auto remote = make_shared<InMemoryDB>("prot_readonly_remote_");
        auto peer = make_shared<RemoteAtomDBPeer>(remote, nullptr, "peer");
        EXPECT_EQ(peer->get_protection_mode(), ProtectionMode::UNPROTECTED);
    }

    // Protected remote backend.
    {
        auto remote = make_shared<ProtectedInMemoryDB>("prot_remote_remote_");
        auto local = make_shared<InMemoryDB>("prot_remote_local_");
        auto peer = make_shared<RemoteAtomDBPeer>(remote, local, "peer");
        EXPECT_EQ(peer->get_protection_mode(), ProtectionMode::PROTECTED);
    }

    // ProtectedAtomDB wrapper as remote backend propagates protection mode.
    {
        auto remote =
            make_shared<ProtectedAtomDB>(make_shared<ProtectedInMemoryDB>("prot_wrapped_remote_"));
        auto peer = make_shared<RemoteAtomDBPeer>(remote, nullptr, "peer");
        EXPECT_EQ(peer->get_protection_mode(), ProtectionMode::PROTECTED);
    }

    // Protected local persistence is not supported.
    {
        auto remote = make_shared<InMemoryDB>("prot_local_remote_");
        auto local = make_shared<ProtectedInMemoryDB>("prot_local_local_");
        EXPECT_THROW(make_shared<RemoteAtomDBPeer>(remote, local, "peer"), runtime_error);
    }

    // FORWARD local persistence is not supported either.
    {
        auto remote = make_shared<InMemoryDB>("forward_local_remote_");
        auto local = make_shared<ForwardInMemoryDB>("forward_local_local_");
        EXPECT_THROW(make_shared<RemoteAtomDBPeer>(remote, local, "peer"), runtime_error);
    }

    // Factory-style ProtectedAtomDB wrapper over a protected backend is rejected for local persistence.
    {
        auto remote = make_shared<InMemoryDB>("wrapped_local_remote_");
        auto local =
            make_shared<ProtectedAtomDB>(make_shared<ProtectedInMemoryDB>("wrapped_local_local_"));
        EXPECT_THROW(make_shared<RemoteAtomDBPeer>(remote, local, "peer"), runtime_error);
    }
}

TEST(RemoteAtomDBFederationTest, IsProtectedWhenAnyPeerIsProtected) {
    // No peers -> nothing to protect.
    {
        map<string, shared_ptr<RemoteAtomDBPeer>> peers;
        auto db = make_shared<RemoteAtomDB>(peers);
        EXPECT_EQ(db->get_protection_mode(), ProtectionMode::UNPROTECTED);
    }

    // All peers unprotected.
    {
        auto remote1 = make_shared<InMemoryDB>("fed_prot_off_remote1_");
        auto remote2 = make_shared<InMemoryDB>("fed_prot_off_remote2_");
        map<string, shared_ptr<RemoteAtomDBPeer>> peers;
        peers["peer1"] = make_shared<RemoteAtomDBPeer>(remote1, nullptr, "peer1");
        peers["peer2"] = make_shared<RemoteAtomDBPeer>(remote2, nullptr, "peer2");
        auto db = make_shared<RemoteAtomDB>(peers);
        EXPECT_EQ(db->get_protection_mode(), ProtectionMode::UNPROTECTED);
    }

    // A single protected peer makes the facade FORWARD (no local post-processing).
    {
        auto unprotected_remote = make_shared<InMemoryDB>("fed_prot_mixed_remote_");
        auto protected_remote = make_shared<ProtectedInMemoryDB>("fed_prot_mixed_protected_");
        map<string, shared_ptr<RemoteAtomDBPeer>> peers;
        peers["unprotected"] = make_shared<RemoteAtomDBPeer>(unprotected_remote, nullptr, "unprotected");
        peers["protected"] = make_shared<RemoteAtomDBPeer>(protected_remote, nullptr, "protected");
        auto db = make_shared<RemoteAtomDB>(peers);
        EXPECT_EQ(db->get_protection_mode(), ProtectionMode::FORWARD);
    }

    // ProtectedAtomDB-wrapped remote peer propagates to the federation facade.
    {
        auto unprotected_remote = make_shared<InMemoryDB>("fed_wrapped_remote_");
        auto protected_remote =
            make_shared<ProtectedAtomDB>(make_shared<ProtectedInMemoryDB>("fed_wrapped_protected_"));
        map<string, shared_ptr<RemoteAtomDBPeer>> peers;
        peers["unprotected"] = make_shared<RemoteAtomDBPeer>(unprotected_remote, nullptr, "unprotected");
        peers["protected"] = make_shared<RemoteAtomDBPeer>(protected_remote, nullptr, "protected");
        auto db = make_shared<RemoteAtomDB>(peers);
        EXPECT_EQ(db->get_protection_mode(), ProtectionMode::FORWARD);
    }
}

TEST(RemoteAtomDBFederationTest, GetAtomForwardsPublicKeyPerPeer) {
    auto protected_backend = make_shared<FullAccessProtectedInMemoryDB>("fed_key_protected_", "admin");
    auto unprotected_backend = make_shared<InMemoryDB>("fed_key_open_");

    auto only_protected = new Node("Symbol", "\"only_protected\"");
    string protected_handle = protected_backend->add_node(only_protected);
    auto only_open = new Node("Symbol", "\"only_open\"");
    string open_handle = unprotected_backend->add_node(only_open);

    map<string, shared_ptr<RemoteAtomDBPeer>> peers;
    peers["peerA"] =
        make_shared<RemoteAtomDBPeer>(make_shared<ProtectedAtomDB>(protected_backend), nullptr, "peerA");
    peers["peerB"] = make_shared<RemoteAtomDBPeer>(unprotected_backend, nullptr, "peerB");
    auto db = make_shared<RemoteAtomDB>(peers);

    EXPECT_EQ(db->get_protection_mode(), ProtectionMode::FORWARD);
    EXPECT_THROW(db->get_atom(protected_handle), runtime_error);

    PublicKey keys(map<string, string>{{"peerA", "admin"}, {"peerB", "unused"}});
    auto from_a = db->get_atom(protected_handle, keys);
    ASSERT_NE(from_a, nullptr);
    EXPECT_EQ(from_a->handle(), protected_handle);

    auto from_b = db->get_atom(open_handle, keys);
    ASSERT_NE(from_b, nullptr);
    EXPECT_EQ(from_b->handle(), open_handle);

    PublicKey denied(map<string, string>{{"peerA", "bad"}, {"peerB", "unused"}});
    EXPECT_EQ(db->get_atom(protected_handle, denied), nullptr);
}

TEST(RemoteAtomDBFederationTest, CacheFirstProbingAcrossPeers) {
    // An atom that exists only in peer2's backend must still resolve via the facade,
    // and the resolving peer must cache it so subsequent probes are served from cache.
    auto backend1 = make_shared<InMemoryDB>("fed_cache_peer1_");
    auto backend2 = make_shared<InMemoryDB>("fed_cache_peer2_");

    auto only_in_peer2 = new Node("Symbol", "\"only_in_peer2\"");
    string handle = backend2->add_node(only_in_peer2);

    map<string, shared_ptr<RemoteAtomDBPeer>> peers;
    peers["peer1"] = make_shared<RemoteAtomDBPeer>(backend1, nullptr, "peer1");
    peers["peer2"] = make_shared<RemoteAtomDBPeer>(backend2, nullptr, "peer2");
    auto db = make_shared<RemoteAtomDB>(peers);

    auto* peer2 = db->get_peer("peer2");
    ASSERT_NE(peer2, nullptr);
    // Before any read, nothing is cached.
    EXPECT_EQ(peer2->get_cached_atom(handle), nullptr);

    // Readonly peers: cache miss then escalate to peer2's backend.
    auto first = db->get_atom(handle);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->handle(), handle);

    // peer2 must have warmed its cache, so a subsequent probe hits.
    EXPECT_NE(peer2->get_cached_atom(handle), nullptr);

    auto second = db->get_atom(handle);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->handle(), handle);

    // A handle present in no peer resolves to nullptr.
    EXPECT_EQ(db->get_atom("ffffffffffffffffffffffffffffffff"), nullptr);
}

TEST(RemoteAtomDBFederationTest, PersistLinkWithoutCrossPeerTargetCopy) {
    // peer3 writable backend + local_persistence start empty; targets live only on peer1.
    // InMemoryDB (and RedisMongoDB with composite_type_enabled=false) can still persist the
    // staged link itself.
    auto peer1_remote = make_shared<InMemoryDB>("persist_peer1_remote_");
    auto peer3_remote = make_shared<InMemoryDB>("persist_peer3_remote_");
    auto peer3_local = make_shared<InMemoryDB>("persist_peer3_local_");

    auto a = new Node("Symbol", "\"persistA\"");
    auto b = new Node("Symbol", "\"persistB\"");
    auto implication = new Node("Symbol", "Implication");
    string a_h = peer1_remote->add_node(a);
    string b_h = peer1_remote->add_node(b);
    string impl_h = peer1_remote->add_node(implication);

    map<string, shared_ptr<RemoteAtomDBPeer>> peers;
    peers["peer1"] = make_shared<RemoteAtomDBPeer>(peer1_remote, nullptr, "peer1");
    peers["peer3"] = make_shared<RemoteAtomDBPeer>(peer3_remote, peer3_local, "peer3");
    auto db = make_shared<RemoteAtomDB>(peers);

    auto link = make_shared<Link>(
        "Expression", vector<string>{impl_h, a_h, b_h}, true, Properties{{"strength", 0.833333}});
    string handle = db->add_link(link.get());
    ASSERT_FALSE(handle.empty());

    EXPECT_FALSE(peer3_local->link_exists(handle));
    EXPECT_FALSE(peer3_local->atom_exists(a_h));
    EXPECT_FALSE(peer3_local->atom_exists(b_h));
    EXPECT_FALSE(peer3_local->atom_exists(impl_h));

    db->get_peer("peer3")->release_cache(true, false);

    ASSERT_TRUE(peer3_local->link_exists(handle));
    EXPECT_FALSE(peer3_local->atom_exists(a_h));
    EXPECT_FALSE(peer3_local->atom_exists(b_h));
    EXPECT_FALSE(peer3_local->atom_exists(impl_h));

    auto persisted = peer3_local->get_atom(handle);
    ASSERT_NE(persisted, nullptr);
    EXPECT_DOUBLE_EQ(persisted->custom_attributes.get_or<double>("strength", -1.0), 0.833333);

    // Fresh facade sharing peer3 local persistence still sees the updated link.
    map<string, shared_ptr<RemoteAtomDBPeer>> reader_peers;
    reader_peers["peer1"] = make_shared<RemoteAtomDBPeer>(peer1_remote, nullptr, "peer1");
    reader_peers["peer3"] = make_shared<RemoteAtomDBPeer>(peer3_remote, peer3_local, "peer3");
    auto reader = make_shared<RemoteAtomDB>(reader_peers);
    auto from_reader = reader->get_atom(handle);
    ASSERT_NE(from_reader, nullptr);
    EXPECT_DOUBLE_EQ(from_reader->custom_attributes.get_or<double>("strength", -1.0), 0.833333);
}

TEST(RemoteAtomDBFederationTest, StrengthUpdateVisibleAcrossPeers) {
    // Writable peer3 local_persistence gets a strength update for a content-addressed handle.
    // Readonly peers may still serve a stale cached/remote copy; facade must prefer peer3.
    auto peer1_remote = make_shared<InMemoryDB>("strength_peer1_remote_");
    auto peer2_remote = make_shared<InMemoryDB>("strength_peer2_remote_");
    auto peer3_remote = make_shared<InMemoryDB>("strength_peer3_remote_");
    auto peer3_local = make_shared<InMemoryDB>("strength_peer3_local_");

    auto a = new Node("Symbol", "\"A\"");
    auto b = new Node("Symbol", "\"B\"");
    auto implication = new Node("Symbol", "Implication");
    string a_h = peer1_remote->add_node(a);
    string b_h = peer1_remote->add_node(b);
    string impl_h = peer1_remote->add_node(implication);
    peer2_remote->add_node(a);
    peer2_remote->add_node(b);
    peer2_remote->add_node(implication);

    auto weak = make_shared<Link>(
        "Expression", vector<string>{impl_h, a_h, b_h}, true, Properties{{"strength", 0.833333}});
    string handle = weak->handle();
    peer1_remote->add_link(weak.get());

    map<string, shared_ptr<RemoteAtomDBPeer>> peers;
    peers["peer1"] = make_shared<RemoteAtomDBPeer>(peer1_remote, nullptr, "peer1");
    peers["peer2"] = make_shared<RemoteAtomDBPeer>(peer2_remote, nullptr, "peer2");
    peers["peer3"] = make_shared<RemoteAtomDBPeer>(peer3_remote, peer3_local, "peer3");
    auto db = make_shared<RemoteAtomDB>(peers);

    // Warm readonly peer1 cache with the weak strength.
    auto first = db->get_atom(handle);
    ASSERT_NE(first, nullptr);
    EXPECT_DOUBLE_EQ(first->custom_attributes.get_or<double>("strength", -1.0), 0.833333);
    EXPECT_NE(db->get_peer("peer1")->get_cached_atom(handle), nullptr);

    // Another process persists a stronger copy into shared peer3 local_persistence.
    auto strong = make_shared<Link>(
        "Expression", vector<string>{impl_h, a_h, b_h}, true, Properties{{"strength", 0.942654}});
    ASSERT_EQ(strong->handle(), handle);
    peer3_local->add_node(a);
    peer3_local->add_node(b);
    peer3_local->add_node(implication);
    peer3_local->add_link(strong.get());

    // Same process still has stale peer1 cache; must still read the updated strength.
    auto updated = db->get_atom(handle);
    ASSERT_NE(updated, nullptr);
    EXPECT_DOUBLE_EQ(updated->custom_attributes.get_or<double>("strength", -1.0), 0.942654);
}

TEST(RemoteAtomDBFederationTest, StagedStrengthUpdateVisibleBeforeFlush) {
    // local_persistence already has a weak copy. A same-process upsert stages a stronger copy
    // in peer3's cache. get_atom must prefer the staged handle over local before release.
    auto peer3_remote = make_shared<InMemoryDB>("staged_strength_peer3_remote_");
    auto peer3_local = make_shared<InMemoryDB>("staged_strength_peer3_local_");

    auto a = new Node("Symbol", "\"stagedA\"");
    auto b = new Node("Symbol", "\"stagedB\"");
    auto implication = new Node("Symbol", "Implication");
    string a_h = peer3_local->add_node(a);
    string b_h = peer3_local->add_node(b);
    string impl_h = peer3_local->add_node(implication);

    auto weak = make_shared<Link>(
        "Expression", vector<string>{impl_h, a_h, b_h}, true, Properties{{"strength", 0.833333}});
    string handle = peer3_local->add_link(weak.get());
    ASSERT_FALSE(handle.empty());

    map<string, shared_ptr<RemoteAtomDBPeer>> peers;
    peers["peer3"] = make_shared<RemoteAtomDBPeer>(peer3_remote, peer3_local, "peer3");
    auto db = make_shared<RemoteAtomDB>(peers);

    auto before = db->get_atom(handle);
    ASSERT_NE(before, nullptr);
    EXPECT_DOUBLE_EQ(before->custom_attributes.get_or<double>("strength", -1.0), 0.833333);

    auto strong = make_shared<Link>(
        "Expression", vector<string>{impl_h, a_h, b_h}, true, Properties{{"strength", 0.942654}});
    ASSERT_EQ(strong->handle(), handle);
    ASSERT_EQ(db->add_link(strong.get()), handle);

    // Still staged only — local_persistence keeps the weak copy until release.
    EXPECT_DOUBLE_EQ(peer3_local->get_atom(handle)->custom_attributes.get_or<double>("strength", -1.0),
                     0.833333);

    auto staged = db->get_atom(handle);
    ASSERT_NE(staged, nullptr);
    EXPECT_DOUBLE_EQ(staged->custom_attributes.get_or<double>("strength", -1.0), 0.942654);

    db->get_peer("peer3")->release_cache(true, false);
    auto flushed = db->get_atom(handle);
    ASSERT_NE(flushed, nullptr);
    EXPECT_DOUBLE_EQ(flushed->custom_attributes.get_or<double>("strength", -1.0), 0.942654);
    EXPECT_DOUBLE_EQ(peer3_local->get_atom(handle)->custom_attributes.get_or<double>("strength", -1.0),
                     0.942654);
}

TEST(RemoteAtomDBFederationTest, NonStagedPrefersLocalOverStaleCache) {
    // Reader warmed a weak copy into cache. Another writer updates local_persistence.
    // get_atom must not keep serving the stale cache entry when the handle is not staged.
    auto peer3_remote = make_shared<InMemoryDB>("stale_cache_peer3_remote_");
    auto peer3_local = make_shared<InMemoryDB>("stale_cache_peer3_local_");

    auto a = new Node("Symbol", "\"staleA\"");
    auto b = new Node("Symbol", "\"staleB\"");
    auto implication = new Node("Symbol", "Implication");
    string a_h = peer3_local->add_node(a);
    string b_h = peer3_local->add_node(b);
    string impl_h = peer3_local->add_node(implication);

    auto weak = make_shared<Link>(
        "Expression", vector<string>{impl_h, a_h, b_h}, true, Properties{{"strength", 0.5}});
    string handle = peer3_local->add_link(weak.get());
    ASSERT_FALSE(handle.empty());

    map<string, shared_ptr<RemoteAtomDBPeer>> peers;
    peers["peer3"] = make_shared<RemoteAtomDBPeer>(peer3_remote, peer3_local, "peer3");
    auto db = make_shared<RemoteAtomDB>(peers);

    auto warmed = db->get_atom(handle);
    ASSERT_NE(warmed, nullptr);
    EXPECT_DOUBLE_EQ(warmed->custom_attributes.get_or<double>("strength", -1.0), 0.5);

    auto strong = make_shared<Link>(
        "Expression", vector<string>{impl_h, a_h, b_h}, true, Properties{{"strength", 0.942654}});
    ASSERT_EQ(strong->handle(), handle);
    ASSERT_EQ(peer3_local->add_link(strong.get()), handle);

    auto refreshed = db->get_atom(handle);
    ASSERT_NE(refreshed, nullptr);
    EXPECT_DOUBLE_EQ(refreshed->custom_attributes.get_or<double>("strength", -1.0), 0.942654);
}

namespace {
// local_persistence stub whose writes fail while `failing` is set. Used to verify that a
// failed flush re-stages dirty atoms instead of losing them.
class FlakyInMemoryDB : public InMemoryDB {
   public:
    explicit FlakyInMemoryDB(const string& context) : InMemoryDB(context) {}

    atomic<bool> failing{false};

    vector<string> add_atoms(const vector<atoms::Atom*>& atoms,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override {
        throw_if_failing();
        return InMemoryDB::add_atoms(atoms, is_transactional, merger);
    }
    vector<string> add_nodes(const vector<atoms::Node*>& nodes,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override {
        throw_if_failing();
        return InMemoryDB::add_nodes(nodes, is_transactional, merger);
    }
    vector<string> add_links(const vector<atoms::Link*>& links,
                             bool is_transactional = false,
                             const atoms::Merger* merger = NULL) override {
        throw_if_failing();
        return InMemoryDB::add_links(links, is_transactional, merger);
    }

   private:
    void throw_if_failing() {
        if (failing.load()) {
            throw runtime_error("simulated backend outage");
        }
    }
};
}  // namespace

TEST(RemoteAtomDBFederationTest, FailedFlushRestagesDirtyAtoms) {
    auto remote = make_shared<InMemoryDB>("flaky_remote_");
    auto local = make_shared<FlakyInMemoryDB>("flaky_local_");
    auto peer = make_shared<RemoteAtomDBPeer>(remote, local, "flaky_peer");

    auto human = new Node("Symbol", "\"human\"");
    string human_handle = peer->add_node(human);
    ASSERT_FALSE(human_handle.empty());

    // Backend down: flush fails, dirty atom must be re-staged, not lost.
    local->failing.store(true);
    peer->release_cache();
    EXPECT_FALSE(local->node_exists(human_handle));
    EXPECT_NE(peer->get_cached_atom(human_handle), nullptr);
    EXPECT_TRUE(peer->node_exists(human_handle));

    // Backend back up: next release persists the re-staged atom.
    local->failing.store(false);
    peer->release_cache();
    EXPECT_TRUE(local->node_exists(human_handle));
    EXPECT_EQ(peer->get_cached_atom(human_handle), nullptr);
}

TEST(RemoteAtomDBFederationTest, ConcurrentAddAndReleaseLosesNoWrites) {
    // Adds racing release_cache() must never drop dirty writes. Every handle lands in
    // local_persistence after a final release once writers finish.
    auto peer3_remote = make_shared<InMemoryDB>("race_peer3_remote_");
    auto peer3_local = make_shared<InMemoryDB>("race_peer3_local_");
    auto peer = make_shared<RemoteAtomDBPeer>(peer3_remote, peer3_local, "peer3");

    constexpr int kWriters = 4;
    constexpr int kAddsPerWriter = 25;
    atomic<bool> start{false};
    // gtest assertions in worker threads only abort that thread; collect failures and
    // assert from the main thread after join.
    atomic<int> add_failures{0};
    vector<thread> writers;
    writers.reserve(kWriters);

    for (int w = 0; w < kWriters; ++w) {
        writers.emplace_back([&, w]() {
            while (!start.load(memory_order_acquire)) {
            }
            for (int i = 0; i < kAddsPerWriter; ++i) {
                auto node =
                    make_unique<Node>("Symbol", "\"race_" + to_string(w) + "_" + to_string(i) + "\"");
                if (peer->add_node(node.get()).empty()) {
                    add_failures.fetch_add(1);
                }
                if ((i % 5) == 0) {
                    peer->release_cache();
                }
            }
        });
    }

    thread releaser([&]() {
        while (!start.load(memory_order_acquire)) {
        }
        for (int i = 0; i < 40; ++i) {
            peer->release_cache();
            this_thread::sleep_for(chrono::milliseconds(1));
        }
    });

    start.store(true, memory_order_release);
    for (auto& t : writers) t.join();
    releaser.join();

    EXPECT_EQ(add_failures.load(), 0);

    // Final flush after all writers finish.
    peer->release_cache();

    for (int w = 0; w < kWriters; ++w) {
        for (int i = 0; i < kAddsPerWriter; ++i) {
            auto probe =
                make_unique<Node>("Symbol", "\"race_" + to_string(w) + "_" + to_string(i) + "\"");
            EXPECT_TRUE(peer3_local->node_exists(probe->handle()))
                << "missing handle for race_" << w << "_" << i;
        }
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
