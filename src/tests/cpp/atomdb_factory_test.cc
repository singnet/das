#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <memory>
#include <string>

#include "AdapterDB.h"
#include "AtomDBFactory.h"
#include "InMemoryDB.h"
#include "JsonConfig.h"
#include "MorkDB.h"
#include "Node.h"
#include "RemoteAtomDB.h"
#include "TestAtomDBJsonConfig.h"
#include "Utils.h"
#include "expression_hasher.h"

using namespace atomdb;
using namespace atoms;
using namespace commons;
using namespace std;

namespace {

JsonConfig config_with_type(const string& type) {
    JsonConfig config;
    config["type"] = type;
    return config;
}

JsonConfig remotedb_config_with_inmemory_peers() {
    nlohmann::json json;
    json["type"] = "remotedb";
    json["remote_peers"] = nlohmann::json::array(
        {{{"uid", "peer1"}, {"type", "inmemorydb"}, {"context", "factory_remote_peer1_"}},
         {{"uid", "peer2"},
          {"type", "inmemorydb"},
          {"context", "factory_remote_peer2_"},
          {"local_persistence", {{"type", "inmemorydb"}, {"context", "factory_remote_peer2_local_"}}}}});
    return JsonConfig(json);
}

}  // namespace

TEST(AtomDBFactoryTest, CreateInMemoryDB) {
    auto db = AtomDBFactory::create(config_with_type("inmemorydb"), "factory_test_");
    ASSERT_NE(db, nullptr);
    EXPECT_NE(dynamic_pointer_cast<InMemoryDB>(db), nullptr);
}

TEST(AtomDBFactoryTest, CreateMorkDB) {
    auto db = AtomDBFactory::create(test_atomdb_json_config("morkdb"), "factory_mork_create_");
    ASSERT_NE(db, nullptr);
    EXPECT_NE(dynamic_pointer_cast<MorkDB>(db), nullptr);
}

TEST(AtomDBFactoryTest, CreateRejectsMissingAndUnknownTypes) {
    EXPECT_THROW(AtomDBFactory::create(JsonConfig()), runtime_error);
    EXPECT_THROW(AtomDBFactory::create(config_with_type("")), runtime_error);
    EXPECT_THROW(AtomDBFactory::create(config_with_type("unknown")), runtime_error);
}

TEST(AtomDBFactoryTest, CreateRemoteAtomDBAssemblesPeers) {
    auto db = AtomDBFactory::create(remotedb_config_with_inmemory_peers(), "");
    ASSERT_NE(db, nullptr);

    auto remote_db = dynamic_pointer_cast<RemoteAtomDB>(db);
    ASSERT_NE(remote_db, nullptr);

    const auto& peers = remote_db->get_remote_dbs();
    EXPECT_EQ(peers.size(), 2u);
    EXPECT_NE(peers.find("peer1"), peers.end());
    EXPECT_NE(peers.find("peer2"), peers.end());
    // peer1 has no local_persistence; peer2 does.
    EXPECT_TRUE(peers.at("peer1")->is_readonly());
    EXPECT_FALSE(peers.at("peer2")->is_readonly());
}

TEST(AtomDBFactoryTest, CreateRemoteAtomDBWithEmptyPeers) {
    JsonConfig config;
    config["type"] = "remotedb";
    config["remote_peers"] = nlohmann::json::array();

    auto db = AtomDBFactory::create(config, "");
    ASSERT_NE(db, nullptr);

    auto remote_db = dynamic_pointer_cast<RemoteAtomDB>(db);
    ASSERT_NE(remote_db, nullptr);
    EXPECT_TRUE(remote_db->get_remote_dbs().empty());
}

TEST(AtomDBFactoryTest, CreateRemoteAtomDBRejectsPeerWithoutUid) {
    nlohmann::json json;
    json["type"] = "remotedb";
    json["remote_peers"] = nlohmann::json::array(
        {{{"type", "inmemorydb"}, {"context", "factory_remote_missing_uid_"}},
         {{"uid", "peer_ok"}, {"type", "inmemorydb"}, {"context", "factory_remote_ok_"}}});

    EXPECT_THROW(AtomDBFactory::create(JsonConfig(json), ""), runtime_error);
}

TEST(AtomDBFactoryTest, CreateAdapterDBRequiresBackendType) {
    JsonConfig missing_backend;
    missing_backend["type"] = "adapterdb";
    missing_backend["adapterdb"] = nlohmann::json::object();

    // Missing adapterdb.atomdb_backend.type makes create_basic_atomdb fail via AtomDB::string_to_type.
    EXPECT_THROW(AtomDBFactory::create(missing_backend, ""), runtime_error);

    // Valid adapterdb.atomdb_backend: factory constructs AdapterDB and delegates AtomDB ops.
    string mapping_path = "/tmp/atomdb_factory_adapterdb_mapping.metta";
    string unique_marker = "factory_adapter_" + to_string(Utils::get_current_time_millis()) + "_" +
                           compute_hash(const_cast<char*>("atomdb_factory_adapterdb"));
    {
        ofstream mapping_file(mapping_path);
        mapping_file << "; " << unique_marker << "\n";
        mapping_file << "(Similarity \"ent\" $h)\n";
        mapping_file << "(Inheritance \"human\" $m)\n";
    }

    auto mork_client = make_shared<MorkClient>("localhost:40032");
    const string similarity_seed = "(Similarity \"ent\" \"human\")";
    const string inheritance_seed = "(Inheritance \"human\" \"mammal\")";
    if (mork_client->get(similarity_seed, similarity_seed).empty()) {
        mork_client->post(similarity_seed);
    }
    if (mork_client->get(inheritance_seed, inheritance_seed).empty()) {
        mork_client->post(inheritance_seed);
    }

    nlohmann::json json;
    json["type"] = "adapterdb";
    json["adapterdb"] = {
        {"type", "mork"},
        {"context_mapping_paths", nlohmann::json::array({mapping_path})},
        {"database_credentials", {{"host", "localhost"}, {"port", 40032}}},
        {"persistence", {{"reuse_mongodb", true}}},
        {"export_metta_on_mapping", {{"enabled", false}, {"output_dir", "/tmp"}}},
        {"atomdb_backend", test_atomdb_json_config("morkdb").get_json()},
    };

    auto db = AtomDBFactory::create(JsonConfig(json), "factory_adapterdb_");
    ASSERT_NE(db, nullptr);

    auto adapter_db = dynamic_pointer_cast<AdapterDB>(db);
    ASSERT_NE(adapter_db, nullptr);

    Node node("Symbol", "FactoryAdapterDBDelegationNode");
    string handle = adapter_db->add_node(&node);
    EXPECT_FALSE(handle.empty());
    EXPECT_TRUE(adapter_db->node_exists(handle));
    ASSERT_NE(adapter_db->get_node(handle), nullptr);

    remove(mapping_path.c_str());
}
