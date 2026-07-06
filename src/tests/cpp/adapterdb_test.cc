#include "AdapterDB.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "AtomDBSingleton.h"
#include "Link.h"
#include "MorkDB.h"
#include "Node.h"
#include "RedisMongoDB.h"
#include "TestAtomDBJsonConfig.h"
#include "Utils.h"
#include "expression_hasher.h"
#include "gmock/gmock.h"

using namespace std;
using namespace atomdb;
using namespace atoms;
using namespace commons;

struct AdapterTestParams {
    string adapter_type;
    string mapping_file_ext;
    string mapping_file_content;
    nlohmann::json db_credentials;
    string test_name;
};

void seed_mork_adapter_test_data() {
    auto mork_client = make_shared<MorkClient>("localhost:40022");

    const string similarity_seed = "(Similarity \"ent\" \"human\")";
    const string inheritance_seed = "(Inheritance \"human\" \"mammal\")";

    const auto similarity_results = mork_client->get(similarity_seed, similarity_seed);
    const auto inheritance_results = mork_client->get(inheritance_seed, inheritance_seed);

    if (similarity_results.empty()) {
        mork_client->post(similarity_seed);
    }
    if (inheritance_results.empty()) {
        mork_client->post(inheritance_seed);
    }
}

class AdapterDBTestBase : public ::testing::Test {
   protected:
    shared_ptr<RedisMongoDB> backend;

    void SetUpBackend() {
        backend = make_shared<RedisMongoDB>("adapter_test", false, test_atomdb_json_config());
    }

    JsonConfig build_adapter_config(const string& mapping_path,
                                    const string& adapter_type,
                                    const nlohmann::json& db_credentials,
                                    const string& backend_type = "morkdb",
                                    bool reuse_mongodb = true) {
        nlohmann::json json;
        json["type"] = "adapterdb";
        json["adapterdb"] = {
            {"type", adapter_type},
            {"context_mapping_paths", nlohmann::json::array({mapping_path})},
            {"database_credentials", db_credentials},
            {"persistence", {{"reuse_mongodb", reuse_mongodb}}},
            {"export_metta_on_mapping", {{"enabled", false}, {"output_dir", "/tmp"}}},
            {"atomdb_backend",
             {
                 {"type", backend_type},
                 {"redis", {{"endpoint", "localhost:40020"}, {"cluster", false}}},
                 {"mongodb",
                  {{"endpoint", "localhost:40021"}, {"username", "admin"}, {"password", "admin"}}},
                 {"morkdb", {{"endpoint", "localhost:40022"}}},
             }},
        };
        return JsonConfig(json);
    }

    shared_ptr<AdapterDB> create_adapter(const string& mapping_path,
                                         const string& adapter_type,
                                         const nlohmann::json& db_credentials,
                                         const string& backend_type = "morkdb",
                                         bool reuse_mongodb = true) {
        auto config = build_adapter_config(
            mapping_path, adapter_type, db_credentials, backend_type, reuse_mongodb);
        return make_shared<AdapterDB>(config, backend);
    }
};

class AdapterDBTest : public AdapterDBTestBase, public ::testing::WithParamInterface<AdapterTestParams> {
   protected:
    string mapping_file_path;

    void SetUp() override {
        SetUpBackend();

        const AdapterTestParams& p = GetParam();
        mapping_file_path = "/tmp/adapterdb_test_mapping" + p.mapping_file_ext;

        string current_time_str = to_string(Utils::get_current_time_millis());
        string test_name = string("AdapterDBTest_") + compute_hash((char*) current_time_str.c_str());

        string comment;

        if (p.adapter_type == "postgres") {
            comment = "-- unique test context: " + test_name + "\n";
        } else if (p.adapter_type == "mork") {
            comment = "; unique test context: " + test_name + "\n";
        }

        ofstream file(mapping_file_path);
        file << comment;
        file << p.mapping_file_content;
        file.close();

        if (p.adapter_type == "mork") {
            seed_mork_adapter_test_data();
        }
    }

    void TearDown() override { remove(mapping_file_path.c_str()); }

    shared_ptr<AdapterDB> create_current_adapter(const string& backend_type = "morkdb",
                                                 bool reuse_mongodb = true) {
        const AdapterTestParams& p = GetParam();
        return create_adapter(
            mapping_file_path, p.adapter_type, p.db_credentials, backend_type, reuse_mongodb);
    }
};

// ---------------------------------------------------------------------------
// Test parameter definitions

static const AdapterTestParams PostgresParams = {
    "postgres",
    ".sql",
    R"(SELECT
        o.organism_id as public_organism__organism_id,
        o.genus as public_organism__genus,
        o.species as public_organism__species,
        o.common_name as public_organism__common_name,
        o.abbreviation as public_organism__abbreviation,
        o.comment as public_organism__comment,
        o.created_at as public_organism__created_at
        FROM
        public.organism as o WHERE o.organism_id=1;

        SELECT
        f.feature_id as public_feature__feature_id,
        f.organism_id as public_feature__organism_id,
        f.type_id as public_feature__type_id,
        f.name as public_feature__name,
        f.uniquename as public_feature__uniquename,
        f.residues as public_feature__residues,
        f.seqlen as public_feature__seqlen,
        f.md5checksum as public_feature__md5checksum,
        f.is_analysis as public_feature__is_analysis,
        f.is_obsolete as public_feature__is_obsolete,
        f.timeaccessioned as public_feature__timeaccessioned,
        f.timelastmodified as public_feature__timelastmodified
        FROM
        public.feature as f WHERE f.feature_id=1;

        SELECT
        c.cvterm_id as public_cvterm__cvterm_id,
        c.name as public_cvterm__name,
        c.definition as public_cvterm__definition,
        c.is_obsolete as public_cvterm__is_obsolete,
        c.is_relationshiptype as public_cvterm__is_relationshiptype,
        c.created_at as public_cvterm__created_at
        FROM
        public.cvterm as c WHERE c.cvterm_id=1;
    )",
    {{"host", "localhost"},
     {"port", 5433},
     {"username", "postgres"},
     {"password", "test"},
     {"database", "postgres_wrapper_test"}},
    "Postgres",
};

static const AdapterTestParams MorkParams = {
    "mork",
    ".metta",
    R"(
(Similarity "ent" $h)
(Inheritance "human" $m))",
    {{"host", "localhost"}, {"port", 40022}},
    "Mork",
};

INSTANTIATE_TEST_SUITE_P(AdapterTypes,
                         AdapterDBTest,
                         ::testing::Values(PostgresParams, MorkParams),
                         [](const ::testing::TestParamInfo<AdapterTestParams>& info) {
                             return info.param.test_name;
                         });

// ---------------------------------------------------------------------------
// Parameterized tests

TEST_P(AdapterDBTest, ConstructorSucceedsWithValidConfig) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);
    EXPECT_GT(db->atom_count(), 0);
}

TEST_P(AdapterDBTest, ConstructorLoadsDataIntoBackendOnFirstRun) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);
    EXPECT_GT(db->atom_count(), 0);
}

TEST_P(AdapterDBTest, CanBeConstructedTwiceWithSameContext) {
    auto db1 = create_current_adapter();
    ASSERT_NE(db1, nullptr);
    EXPECT_GT(db1->atom_count(), 0);

    EXPECT_NO_THROW({
        auto db2 = create_current_adapter();
        ASSERT_NE(db2, nullptr);
        EXPECT_GT(db2->atom_count(), 0);
    });
}

TEST_P(AdapterDBTest, ReloadDoesNotThrowAndKeepsBackendUsable) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);

    EXPECT_NO_THROW({ db->reload(); });

    EXPECT_NO_THROW({
        auto count = db->atom_count();
        (void) count;
    });

    EXPECT_NO_THROW({
        bool nested = db->allow_nested_indexing();
        (void) nested;
    });
}

TEST_P(AdapterDBTest, NeedsSyncIsNotImplemented) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);
    EXPECT_THROW({ db->needs_sync(); }, runtime_error);
}

TEST_P(AdapterDBTest, AddGetAndDeleteNode) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);

    auto node = new Node("Symbol", "AdapterDBTestNode");
    string handle = db->add_node(node);

    EXPECT_NE(handle, "");
    EXPECT_TRUE(db->node_exists(handle));
    EXPECT_TRUE(db->atom_exists(handle));

    auto fetched_atom = db->get_atom(handle);
    auto fetched_node = db->get_node(handle);
    auto fetched_link = db->get_link(handle);

    ASSERT_NE(fetched_atom, nullptr);
    ASSERT_NE(fetched_node, nullptr);
    EXPECT_EQ(fetched_link, nullptr);

    EXPECT_TRUE(db->delete_node(handle));
    EXPECT_FALSE(db->node_exists(handle));

    delete node;
}

TEST_P(AdapterDBTest, AddAndDeleteNodes) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);

    vector<Node*> nodes;
    nodes.push_back(new Node("Symbol", "AdapterDBNode1"));
    nodes.push_back(new Node("Symbol", "AdapterDBNode2"));
    nodes.push_back(new Node("Symbol", "AdapterDBNode3"));

    auto handles = db->add_nodes(nodes);
    EXPECT_EQ(handles.size(), 3);
    EXPECT_EQ(db->nodes_exist(handles).size(), 3);

    EXPECT_EQ(db->delete_nodes(handles), 3);
    EXPECT_EQ(db->nodes_exist(handles).size(), 0);

    for (auto* n : nodes) delete n;
}

TEST_P(AdapterDBTest, AddGetAndQueryTargetsLink) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);

    auto n1 = new Node("Symbol", "AdapterLinkNode1");
    auto n2 = new Node("Symbol", "AdapterLinkNode2");
    auto n3 = new Node("Symbol", "AdapterLinkNode3");

    auto h1 = db->add_node(n1);
    auto h2 = db->add_node(n2);
    auto h3 = db->add_node(n3);

    auto link = new Link("Expression", {h1, h2, h3});
    auto link_handle = db->add_link(link);

    EXPECT_NE(link_handle, "");
    EXPECT_TRUE(db->link_exists(link_handle));
    EXPECT_TRUE(db->atom_exists(link_handle));

    auto fetched_atom = db->get_atom(link_handle);
    auto fetched_link = db->get_link(link_handle);
    auto fetched_node = db->get_node(link_handle);

    ASSERT_NE(fetched_atom, nullptr);
    ASSERT_NE(fetched_link, nullptr);
    EXPECT_EQ(fetched_node, nullptr);

    auto targets = db->query_for_targets(link_handle);
    ASSERT_NE(targets, nullptr);
    EXPECT_EQ(targets->size(), 3);

    EXPECT_TRUE(db->delete_link(link_handle));
    EXPECT_TRUE(db->delete_node(h1));
    EXPECT_TRUE(db->delete_node(h2));
    EXPECT_TRUE(db->delete_node(h3));

    delete n1;
    delete n2;
    delete n3;
    delete link;
}

TEST_P(AdapterDBTest, AddLinkFailsWhenTargetsDoNotExist) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);

    auto fake1 = new Node("Symbol", "AdapterLinkFailNode1");
    auto fake2 = new Node("Symbol", "AdapterLinkFailNode2");
    auto fake3 = new Node("Symbol", "AdapterLinkFailNode3");

    auto link = new Link("Expression", {fake1->handle(), fake2->handle(), fake3->handle()});

    EXPECT_THROW({ db->add_link(link); }, runtime_error);

    delete fake1;
    delete fake2;
    delete fake3;
    delete link;
}

TEST_P(AdapterDBTest, DeleteNonExistingAtomReturnsFalse) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);

    EXPECT_FALSE(db->delete_atom("NonExistingHandle"));
    EXPECT_FALSE(db->delete_node("NonExistingHandle"));
    EXPECT_FALSE(db->delete_link("NonExistingHandle"));
}

TEST_P(AdapterDBTest, DeleteEmptyCollectionsReturnsZero) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);

    EXPECT_EQ(db->delete_atoms({}), 0);
    EXPECT_EQ(db->delete_nodes({}), 0);
    EXPECT_EQ(db->delete_links({}), 0);
}

TEST_P(AdapterDBTest, ExistsQueriesReturnEmptyForUnknownHandles) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);

    vector<string> handles = {
        "NonExistingHandle1",
        "NonExistingHandle2",
        "NonExistingHandle3",
    };

    EXPECT_EQ(db->atoms_exist(handles).size(), 0);
    EXPECT_EQ(db->nodes_exist(handles).size(), 0);
    EXPECT_EQ(db->links_exist(handles).size(), 0);
}

TEST_P(AdapterDBTest, ReIndexPatternsDoesNotThrow) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);
    EXPECT_NO_THROW({ db->re_index_patterns(); });
}

TEST_P(AdapterDBTest, AddNodeWithThrowIfExists) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);

    auto node = new Node("Symbol", "AdapterThrowIfExistsNode");

    EXPECT_EQ(db->add_node(node, true), node->handle());
    EXPECT_THROW({ db->add_node(node, true); }, runtime_error);

    EXPECT_TRUE(db->delete_node(node->handle()));
    delete node;
}

TEST_P(AdapterDBTest, AddLinkWithThrowIfExists) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);

    auto n1 = new Node("Symbol", "AdapterThrowIfExistsLink1");
    auto n2 = new Node("Symbol", "AdapterThrowIfExistsLink2");
    auto n3 = new Node("Symbol", "AdapterThrowIfExistsLink3");

    auto h1 = db->add_node(n1);
    auto h2 = db->add_node(n2);
    auto h3 = db->add_node(n3);

    auto link = new Link("Expression", {h1, h2, h3});

    EXPECT_EQ(db->add_link(link), link->handle());
    EXPECT_THROW({ db->add_link(link, true); }, runtime_error);

    EXPECT_TRUE(db->delete_link(link->handle()));
    EXPECT_TRUE(db->delete_node(h1));
    EXPECT_TRUE(db->delete_node(h2));
    EXPECT_TRUE(db->delete_node(h3));

    delete n1;
    delete n2;
    delete n3;
    delete link;
}

TEST_P(AdapterDBTest, AddSameNodeWithoutThrowIfExistsIsAccepted) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);

    auto node = new Node("Symbol", "AdapterSameNode");

    EXPECT_EQ(db->add_node(node), node->handle());
    EXPECT_EQ(db->add_node(node), node->handle());

    EXPECT_TRUE(db->node_exists(node->handle()));
    EXPECT_TRUE(db->delete_node(node->handle()));
    EXPECT_FALSE(db->node_exists(node->handle()));

    delete node;
}

TEST_P(AdapterDBTest, AddSameLinkWithoutThrowIfExistsIsAccepted) {
    auto db = create_current_adapter();
    ASSERT_NE(db, nullptr);

    auto n1 = new Node("Symbol", "AdapterSameLink1");
    auto n2 = new Node("Symbol", "AdapterSameLink2");
    auto n3 = new Node("Symbol", "AdapterSameLink3");

    auto h1 = db->add_node(n1);
    auto h2 = db->add_node(n2);
    auto h3 = db->add_node(n3);

    auto link = new Link("Expression", {h1, h2, h3});

    EXPECT_EQ(db->add_link(link), link->handle());
    EXPECT_EQ(db->add_link(link), link->handle());

    EXPECT_TRUE(db->delete_link(link->handle()));
    EXPECT_TRUE(db->delete_node(h1));
    EXPECT_TRUE(db->delete_node(h2));
    EXPECT_TRUE(db->delete_node(h3));

    delete n1;
    delete n2;
    delete n3;
    delete link;
}

class AdapterDBConstructorFailureTest : public AdapterDBTestBase {
   protected:
    string mapping_file_path;

    void SetUp() override {
        SetUpBackend();

        mapping_file_path = "/tmp/adapterdb_failure_test_mapping.sql";

        ofstream file(mapping_file_path);
        file << "SELECT o.organism_id FROM public.organism AS o WHERE o.organism_id=1;\n";
        file.close();
    }

    void TearDown() override { remove(mapping_file_path.c_str()); }

    shared_ptr<AdapterDB> create_postgres_adapter(const string& backend_type = "morkdb",
                                                  bool reuse_mongodb = true) {
        nlohmann::json creds = {{"host", "localhost"},
                                {"port", 5433},
                                {"username", "postgres"},
                                {"password", "test"},
                                {"database", "postgres_wrapper_test"}};
        return create_adapter(mapping_file_path, "postgres", creds, backend_type, reuse_mongodb);
    }
};

TEST_F(AdapterDBConstructorFailureTest, ConstructorFailsWithInvalidAdapterType) {
    nlohmann::json creds = {{"host", "localhost"},
                            {"port", 5433},
                            {"username", "postgres"},
                            {"password", "test"},
                            {"database", "postgres_wrapper_test"}};
    EXPECT_THROW({ create_adapter(mapping_file_path, "mysql", creds); }, runtime_error);
}

TEST_F(AdapterDBConstructorFailureTest, ConstructorFailsWhenPersistenceReuseMongoIsFalse) {
    EXPECT_THROW({ create_postgres_adapter("morkdb", false); }, runtime_error);
}

TEST_F(AdapterDBConstructorFailureTest, ConstructorFailsWithUnsupportedBackendForMongoPersistence) {
    EXPECT_THROW({ create_postgres_adapter("remotedb", true); }, runtime_error);
}

TEST_F(AdapterDBConstructorFailureTest, ConstructorFailsWhenMappingFileDoesNotExist) {
    nlohmann::json creds = {{"host", "localhost"},
                            {"port", 5433},
                            {"username", "postgres"},
                            {"password", "test"},
                            {"database", "postgres_wrapper_test"}};
    EXPECT_THROW({ create_adapter("/tmp/does_not_exist_adapterdb_mapping.sql", "postgres", creds); },
                 runtime_error);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}