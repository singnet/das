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
#include "TestAtomDBJsonConfig.h"
#include "gmock/gmock.h"

using namespace std;
using namespace atomdb;
using namespace atoms;
using namespace commons;

class AdapterDBTest : public ::testing::Test {
   protected:
    string mapping_file_path;

    void SetUp() override {
        mapping_file_path = "/tmp/adapterdb_test_mapping.sql";

        ofstream file(mapping_file_path);
        file << R"(SELECT
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
        )";
        file.close();
    }

    void TearDown() override { remove(mapping_file_path.c_str()); }

    JsonConfig build_adapter_config(const string& mapping_path,
                                    const string& backend_type = "morkdb",
                                    bool reuse_mongodb = true,
                                    const string& adapter_type = "postgres") {
        nlohmann::json json;

        json["type"] = "adapterdb";

        json["adapterdb"] = {
            {"type", adapter_type},
            {"context_mapping_paths", nlohmann::json::array({mapping_path})},
            {"database_credentials",
             {
                 {"host", "localhost"},
                 {"port", 5433},
                 {"username", "postgres"},
                 {"password", "test"},
                 {"database", "postgres_wrapper_test"},
             }},
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
                                         const string& backend_type = "morkdb",
                                         bool reuse_mongodb = true,
                                         const string& adapter_type = "postgres") {
        auto config = build_adapter_config(mapping_path, backend_type, reuse_mongodb, adapter_type);
        auto backend = make_shared<MorkDB>("adapterdb", test_atomdb_json_config("morkdb"));
        return make_shared<AdapterDB>(config, backend);
    }
};

TEST_F(AdapterDBTest, ConstructorSucceedsWithValidConfig) {
    auto db = create_adapter(mapping_file_path);
    ASSERT_NE(db, nullptr);
    EXPECT_GT(db->atom_count(), 0);
}

TEST_F(AdapterDBTest, ConstructorFailsWithInvalidAdapterType) {
    EXPECT_THROW({ auto db = create_adapter(mapping_file_path, "morkdb", true, "mysql"); },
                 runtime_error);
}

TEST_F(AdapterDBTest, ConstructorFailsWhenPersistenceReuseMongoIsFalse) {
    EXPECT_THROW({ auto db = create_adapter(mapping_file_path, "morkdb", false, "postgres"); },
                 runtime_error);
}

TEST_F(AdapterDBTest, ConstructorFailsWithUnsupportedBackendForMongoPersistence) {
    EXPECT_THROW({ auto db = create_adapter(mapping_file_path, "remotedb", true, "postgres"); },
                 runtime_error);
}

TEST_F(AdapterDBTest, ConstructorFailsWhenMappingFileDoesNotExist) {
    EXPECT_THROW({ auto db = create_adapter("/tmp/does_not_exist_adapterdb_mapping.json"); },
                 runtime_error);
}

TEST_F(AdapterDBTest, ConstructorLoadsDataIntoBackendOnFirstRun) {
    auto db = create_adapter(mapping_file_path);
    ASSERT_NE(db, nullptr);

    EXPECT_GT(db->atom_count(), 0);
    EXPECT_GT(db->node_count(), 0);
}

TEST_F(AdapterDBTest, CanBeConstructedTwiceWithSameContext) {
    auto db1 = create_adapter(mapping_file_path);
    ASSERT_NE(db1, nullptr);
    EXPECT_GT(db1->atom_count(), 0);

    EXPECT_NO_THROW({
        auto db2 = create_adapter(mapping_file_path);
        ASSERT_NE(db2, nullptr);
        EXPECT_GT(db2->atom_count(), 0);
    });
}

TEST_F(AdapterDBTest, ReloadDoesNotThrowAndKeepsBackendUsable) {
    auto db = create_adapter(mapping_file_path);
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

TEST_F(AdapterDBTest, NeedsSyncIsNotImplemented) {
    auto db = create_adapter(mapping_file_path);
    ASSERT_NE(db, nullptr);

    EXPECT_THROW({ db->needs_sync(); }, runtime_error);
}

TEST_F(AdapterDBTest, AddGetAndDeleteNode) {
    auto db = create_adapter(mapping_file_path);
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

TEST_F(AdapterDBTest, AddAndDeleteNodes) {
    auto db = create_adapter(mapping_file_path);
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

TEST_F(AdapterDBTest, AddGetAndQueryTargetsLink) {
    auto db = create_adapter(mapping_file_path);
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

    EXPECT_TRUE(db->delete_node(h1));
    EXPECT_TRUE(db->delete_node(h2));
    EXPECT_TRUE(db->delete_node(h3));

    delete n1;
    delete n2;
    delete n3;
    delete link;
}

TEST_F(AdapterDBTest, AddLinkFailsWhenTargetsDoNotExist) {
    auto db = create_adapter(mapping_file_path);
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

TEST_F(AdapterDBTest, DeleteNonExistingAtomReturnsFalse) {
    auto db = create_adapter(mapping_file_path);
    ASSERT_NE(db, nullptr);

    EXPECT_FALSE(db->delete_atom("NonExistingHandle"));
    EXPECT_FALSE(db->delete_node("NonExistingHandle"));
    EXPECT_FALSE(db->delete_link("NonExistingHandle"));
}

TEST_F(AdapterDBTest, DeleteEmptyCollectionsReturnsZero) {
    auto db = create_adapter(mapping_file_path);
    ASSERT_NE(db, nullptr);

    EXPECT_EQ(db->delete_atoms({}), 0);
    EXPECT_EQ(db->delete_nodes({}), 0);
    EXPECT_EQ(db->delete_links({}), 0);
}

TEST_F(AdapterDBTest, ExistsQueriesReturnEmptyForUnknownHandles) {
    auto db = create_adapter(mapping_file_path);
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

TEST_F(AdapterDBTest, ReIndexPatternsDoesNotThrow) {
    auto db = create_adapter(mapping_file_path);
    ASSERT_NE(db, nullptr);

    EXPECT_NO_THROW({ db->re_index_patterns(); });
}

TEST_F(AdapterDBTest, AddNodeWithThrowIfExists) {
    auto db = create_adapter(mapping_file_path);
    ASSERT_NE(db, nullptr);

    auto node = new Node("Symbol", "AdapterThrowIfExistsNode");

    EXPECT_EQ(db->add_node(node, true), node->handle());
    EXPECT_THROW({ db->add_node(node, true); }, runtime_error);

    EXPECT_TRUE(db->delete_node(node->handle()));
    delete node;
}

TEST_F(AdapterDBTest, AddLinkWithThrowIfExists) {
    auto db = create_adapter(mapping_file_path);
    ASSERT_NE(db, nullptr);

    auto n1 = new Node("Symbol", "AdapterThrowIfExistsLink1");
    auto n2 = new Node("Symbol", "AdapterThrowIfExistsLink2");
    auto n3 = new Node("Symbol", "AdapterThrowIfExistsLink3");

    auto h1 = db->add_node(n1);
    auto h2 = db->add_node(n2);
    auto h3 = db->add_node(n3);

    auto link = new Link("Expression", {h1, h2, h3});

    EXPECT_EQ(db->add_link(link, true), link->handle());
    EXPECT_THROW({ db->add_link(link, true); }, runtime_error);

    EXPECT_TRUE(db->delete_node(h1));
    EXPECT_TRUE(db->delete_node(h2));
    EXPECT_TRUE(db->delete_node(h3));

    delete n1;
    delete n2;
    delete n3;
    delete link;
}

TEST_F(AdapterDBTest, AddSameNodeWithoutThrowIfExistsIsAccepted) {
    auto db = create_adapter(mapping_file_path);
    ASSERT_NE(db, nullptr);

    auto node = new Node("Symbol", "AdapterSameNode");

    EXPECT_EQ(db->add_node(node), node->handle());
    EXPECT_EQ(db->add_node(node), node->handle());

    EXPECT_TRUE(db->node_exists(node->handle()));
    EXPECT_TRUE(db->delete_node(node->handle()));
    EXPECT_FALSE(db->node_exists(node->handle()));

    delete node;
}

TEST_F(AdapterDBTest, AddSameLinkWithoutThrowIfExistsIsAccepted) {
    auto db = create_adapter(mapping_file_path);
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

    EXPECT_TRUE(db->delete_node(h1));
    EXPECT_TRUE(db->delete_node(h2));
    EXPECT_TRUE(db->delete_node(h3));

    delete n1;
    delete n2;
    delete n3;
    delete link;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
