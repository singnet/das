#include <gtest/gtest.h>

#include <algorithm>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <memory>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "AuthorizationManifest.h"
#include "AuthorizationPersistence.h"
#include "InMemoryAccessPermissionTypes.h"
#include "InMemoryDB.h"
#include "Link.h"
#include "LinkSchema.h"
#include "MongodbAuthorizationPersistence.h"
#include "Node.h"
#include "RedisMongoDBAPITypes.h"
#include "TestAtomDBJsonConfig.h"

using namespace atomdb;
using namespace atomdb_api_types;
using namespace atoms;
using namespace std;

namespace {

vector<string> inheritance_mammal_tokens() {
    return {"LINK_TEMPLATE",
            "Expression",
            "3",
            "NODE",
            "Symbol",
            "Inheritance",
            "VARIABLE",
            "x",
            "NODE",
            "Symbol",
            "\"mammal\""};
}

pair<LinkSchema, unsigned int> read_only_inheritance_schema() {
    return {LinkSchema(inheritance_mammal_tokens()), 1};
}

shared_ptr<AccessPermissionDocument> make_document(const string& access_key,
                                                   bool full_access,
                                                   vector<pair<LinkSchema, unsigned int>> schemas) {
    auto document = make_shared<InMemoryAccessPermissionDocument>();
    document->set_access_key(access_key);
    document->set_full_access(full_access);
    for (auto& [schema, permission] : schemas) {
        document->append_entry(schema.tokens(), permission & 1, permission & 2);
    }
    return document;
}

class DummyPersistence : public AuthorizationPersistence {
   public:
    map<string, vector<pair<LinkSchema, unsigned int>>> documents;

    void authorize(const string& public_key, vector<pair<LinkSchema, unsigned int>>& schemas) override {
        auto& entries = documents[public_key];
        entries.insert(entries.end(), schemas.begin(), schemas.end());
    }
    void authorize(const string& public_key) override { documents[public_key] = {}; }

    void revoke(const string& public_key) override { documents.erase(public_key); }
};

class TestManifest : public AuthorizationManifest {
   public:
    using AuthorizationManifest::add_document;
    using AuthorizationManifest::AuthorizationManifest;
};

shared_ptr<InMemoryDB> db_with_inheritance_link(string* link_handle) {
    auto db = make_shared<InMemoryDB>("auth_test_");
    auto human = new Node("Symbol", "\"human\"");
    auto mammal = new Node("Symbol", "\"mammal\"");
    auto inheritance = new Node("Symbol", "Inheritance");
    string human_handle = db->add_node(human);
    string mammal_handle = db->add_node(mammal);
    string inheritance_handle = db->add_node(inheritance);
    auto link = new Link("Expression", {inheritance_handle, human_handle, mammal_handle});
    *link_handle = db->add_link(link);
    delete human;
    delete mammal;
    delete inheritance;
    delete link;
    return db;
}

TestManifest manifest_from_persistence(shared_ptr<AtomDB> atomdb, const DummyPersistence& persistence) {
    TestManifest manifest(atomdb);
    for (const auto& [public_key, entries] : persistence.documents) {
        manifest.add_document(make_document(public_key, false, entries));
    }
    return manifest;
}

unique_ptr<MongodbAuthorizationPersistence> make_mongo_persistence(const string& database_name,
                                                                   const string& collection_name,
                                                                   const string& uri_query = "") {
    auto config = test_atomdb_json_config();
    string endpoint = config.at_path("mongodb.endpoint").get_or<string>("localhost:40021");
    if (!uri_query.empty()) {
        endpoint += "/?" + uri_query;
    }
    string username = config.at_path("mongodb.username").get_or<string>("admin");
    string password = config.at_path("mongodb.password").get_or<string>("admin");
    return make_unique<MongodbAuthorizationPersistence>(
        endpoint, username, password, database_name, collection_name);
}

}  // namespace

TEST(AuthorizationManifestTest, BuildsProfilesFromDocuments) {
    TestManifest manifest(nullptr);
    manifest.add_document(make_document("pk1", false, {read_only_inheritance_schema()}));
    manifest.add_document(make_document("pk2", true, {}));

    EXPECT_TRUE(manifest.is_registered("pk1"));
    EXPECT_TRUE(manifest.is_registered("pk2"));
    EXPECT_FALSE(manifest.is_registered("unknown"));
}

TEST(AuthorizationManifestTest, EmptyDocuments) {
    AuthorizationManifest manifest(nullptr);
    EXPECT_FALSE(manifest.is_registered("pk"));
}

TEST(AuthorizationManifestTest, IsAuthorized) {
    string link_handle;
    auto db = db_with_inheritance_link(&link_handle);

    TestManifest manifest(db);
    manifest.add_document(make_document("pk", false, {read_only_inheritance_schema()}));

    auto link = db->get_link(link_handle);
    ASSERT_NE(link, nullptr);

    EXPECT_FALSE(manifest.is_registered("unknown"));

    EXPECT_FALSE(manifest.is_authorized(link, "unknown", AuthorizationOperation::READ));
    EXPECT_FALSE(manifest.is_authorized(link, "unknown", AuthorizationOperation::WRITE));
    EXPECT_FALSE(manifest.is_authorized(link_handle, "unknown", AuthorizationOperation::READ));
    EXPECT_FALSE(manifest.is_authorized(link_handle, "unknown", AuthorizationOperation::WRITE));
    EXPECT_FALSE(manifest.is_registered("unknown"));

    EXPECT_TRUE(manifest.is_authorized(link, "pk", AuthorizationOperation::READ));
    EXPECT_TRUE(manifest.is_authorized(link_handle, "pk", AuthorizationOperation::READ));

    EXPECT_FALSE(manifest.is_authorized(link, "pk", AuthorizationOperation::WRITE));
    EXPECT_FALSE(manifest.is_authorized(link_handle, "pk", AuthorizationOperation::WRITE));
}

TEST(AuthorizationManifestTest, FullAccessGrantsAllOperations) {
    string link_handle;
    auto db = db_with_inheritance_link(&link_handle);

    TestManifest manifest(db);
    manifest.add_document(make_document("pk", true, {}));

    auto link = db->get_link(link_handle);
    ASSERT_NE(link, nullptr);

    EXPECT_TRUE(manifest.is_authorized(link, "pk", AuthorizationOperation::READ));
    EXPECT_TRUE(manifest.is_authorized(link, "pk", AuthorizationOperation::WRITE));
    EXPECT_TRUE(manifest.is_authorized(link_handle, "pk", AuthorizationOperation::WRITE));
}

TEST(AuthorizationProfileTest, FromDocumentWithAndWithoutSchema) {
    auto db = make_shared<InMemoryDB>("auth_test_");
    auto document = make_document("pk", false, {read_only_inheritance_schema()});
    auto profile = AuthorizationProfile::from_document(db, document);

    EXPECT_FALSE(profile->is_full_access());
}

TEST(AuthorizationProfileTest, FullAccessRejectsSchemas) {
    auto db = make_shared<InMemoryDB>("auth_test_");
    auto document = make_document("pk", true, {});
    auto profile = AuthorizationProfile::from_document(db, document);
    EXPECT_TRUE(profile->is_full_access());

    vector<shared_ptr<AuthorizationSchema>> schemas{
        make_shared<AuthorizationSchema>(db, inheritance_mammal_tokens(), true, false)};
    EXPECT_THROW(AuthorizationProfile(true, schemas), runtime_error);
}

TEST(AuthorizationPersistenceTest, ManifestReflectsPersistedPermissions) {
    string link_handle;
    auto db = db_with_inheritance_link(&link_handle);

    auto persistence = make_shared<DummyPersistence>();
    auto schema = read_only_inheritance_schema();
    vector<pair<LinkSchema, unsigned int>> schemas{schema};

    persistence->authorize("pk", schemas);

    TestManifest manifest = manifest_from_persistence(db, *persistence);
    auto link = db->get_link(link_handle);
    ASSERT_NE(link, nullptr);

    EXPECT_TRUE(manifest.is_authorized(link, "pk", AuthorizationOperation::READ));
    EXPECT_FALSE(manifest.is_authorized(link, "pk", AuthorizationOperation::WRITE));
}

TEST(AuthorizationPersistenceTest, AuthorizeThenReadAndWriteFlags) {
    auto persistence = make_shared<DummyPersistence>();
    auto schema = read_only_inheritance_schema();
    vector<pair<LinkSchema, unsigned int>> schemas{schema};

    persistence->authorize("pk", schemas);
    persistence->authorize("pk2", schemas);

    EXPECT_EQ(persistence->documents.size(), 2u);
    ASSERT_EQ(persistence->documents["pk"].size(), 1u);
    ASSERT_EQ(persistence->documents["pk2"].size(), 1u);
    EXPECT_EQ(persistence->documents["pk"][0].second, 1u);
    EXPECT_EQ(persistence->documents["pk2"][0].second, 1u);

    persistence->revoke("pk");
    EXPECT_EQ(persistence->documents.size(), 1u);
    EXPECT_EQ(persistence->documents.count("pk"), 0u);
    EXPECT_EQ(persistence->documents["pk2"].size(), 1u);
}

TEST(MongodbAuthorizationPersistenceTest, RemoveLastSchemaDeletesDocument) {
    auto config = test_atomdb_json_config();
    string endpoint = config.at_path("mongodb.endpoint").get_or<string>("localhost:40021");
    string username = config.at_path("mongodb.username").get_or<string>("admin");
    string password = config.at_path("mongodb.password").get_or<string>("admin");
    const string database_name = "auth_remove_regression_das";
    const string collection_name = "access_permissions";
    const string public_key = "pk_remove_last_schema";

    auto persistence = make_mongo_persistence(database_name, collection_name);
    persistence->revoke(public_key);

    vector<pair<LinkSchema, unsigned int>> schemas{read_only_inheritance_schema()};
    persistence->authorize(public_key, schemas);
    persistence->revoke(public_key);

    mongocxx::client client{mongocxx::uri{"mongodb://" + username + ":" + password + "@" + endpoint}};
    auto collection = client[database_name][collection_name];
    auto reply = collection.find_one(
        bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("public_key", public_key)));
    EXPECT_FALSE(static_cast<bool>(reply));

    AuthorizationManifest manifest(nullptr);
    EXPECT_FALSE(manifest.is_registered(public_key));
}

TEST(MongodbAuthorizationPersistenceTest, RemoveDoesNotDeadlockWithMaxPoolSize1) {
    const string database_name = "auth_remove_pool_regression_das";
    const string collection_name = "access_permissions";
    const string public_key = "pk_max_pool_size_1";

    auto persistence = make_mongo_persistence(database_name, collection_name, "maxPoolSize=1");
    persistence->revoke(public_key);

    vector<pair<LinkSchema, unsigned int>> schemas{read_only_inheritance_schema()};
    persistence->authorize(public_key, schemas);
    persistence->revoke(public_key);
    persistence->revoke(public_key);
}

TEST(MongodbAuthorizationPersistenceTest, RemoveAllRaisesOnUnacknowledgedDelete) {
    const string database_name = "auth_remove_w0_regression_das";
    const string collection_name = "access_permissions";
    const string public_key = "pk_unacked_delete";

    auto setup = make_mongo_persistence(database_name, collection_name);
    setup->revoke(public_key);
    vector<pair<LinkSchema, unsigned int>> schemas{read_only_inheritance_schema()};
    setup->authorize(public_key, schemas);

    auto unacked = make_mongo_persistence(database_name, collection_name, "w=0");
    EXPECT_THROW(
        {
            try {
                unacked->revoke(public_key);
            } catch (const runtime_error& error) {
                EXPECT_STREQ(error.what(), "Failed to remove authorization document from MongoDB");
                throw;
            }
        },
        runtime_error);

    setup->revoke(public_key);
}
