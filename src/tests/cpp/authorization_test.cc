#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "AuthorizationManager.h"
#include "AuthorizationManifest.h"
#include "AuthorizationPersistence.h"
#include "InMemoryAccessPermissionTypes.h"
#include "InMemoryDB.h"
#include "Link.h"
#include "LinkSchema.h"
#include "Node.h"

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

AuthorizationSchema read_only_inheritance_schema() {
    return AuthorizationSchema(inheritance_mammal_tokens(), true, false);
}

shared_ptr<AccessPermissionDocument> make_document(const string& access_key,
                                                   bool full_access,
                                                   const vector<AuthorizationSchema>& schemas) {
    auto document = make_shared<InMemoryAccessPermissionDocument>();
    document->set_access_key(access_key);
    document->set_full_access(full_access);
    for (const auto& schema : schemas) {
        document->append_entry(LinkSchema(schema.schema()).tokenize(), schema.read(), schema.write());
    }
    return document;
}

class DummyPersistence : public AuthorizationPersistence {
   public:
    map<string, vector<AuthorizationSchema>> documents;

    vector<AuthorizationSchema> list(const string& public_key) override {
        auto it = documents.find(public_key);
        if (it == documents.end()) {
            return {};
        }
        return it->second;
    }

    void save(const string& public_key, const AuthorizationSchema& entry) override {
        auto& entries = documents[public_key];
        for (auto& existing : entries) {
            if (existing.schema().handle() == entry.schema().handle()) {
                existing = entry;
                return;
            }
        }
        entries.push_back(entry);
    }

    void remove(const string& public_key, const AuthorizationSchema& entry) override {
        auto it = documents.find(public_key);
        if (it == documents.end()) {
            return;
        }

        auto& entries = it->second;
        auto handle = entry.schema().handle();
        entries.erase(remove_if(entries.begin(),
                                entries.end(),
                                [&handle](const AuthorizationSchema& existing) {
                                    return existing.schema().handle() == handle;
                                }),
                      entries.end());
    }

    void remove_all(const string& public_key) override { documents.erase(public_key); }
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

AuthorizationManifest manifest_from_persistence(const DummyPersistence& persistence) {
    AuthorizationManifest manifest;
    for (const auto& [public_key, entries] : persistence.documents) {
        manifest.add_document(make_document(public_key, false, entries));
    }
    return manifest;
}

}  // namespace

TEST(AuthorizationManifestTest, BuildsProfilesFromDocuments) {
    AuthorizationManifest manifest;
    manifest.add_document(make_document("pk1", false, {read_only_inheritance_schema()}));
    manifest.add_document(make_document("pk2", true, {}));

    EXPECT_TRUE(manifest.is_registered("pk1"));
    EXPECT_TRUE(manifest.is_registered("pk2"));
    EXPECT_FALSE(manifest.is_registered("unknown"));

    EXPECT_FALSE(manifest.full_access("pk1"));
    EXPECT_TRUE(manifest.full_access("pk2"));

    auto profile = manifest.lookup("pk1");
    ASSERT_NE(profile, nullptr);
    EXPECT_FALSE(profile->is_full_access());

    const auto& schemas = profile->schemas();
    ASSERT_EQ(schemas.size(), 1u);
    EXPECT_TRUE(schemas[0].allows(AuthorizationOperation::READ));
    EXPECT_FALSE(schemas[0].allows(AuthorizationOperation::WRITE));
    EXPECT_EQ(schemas[0].schema().handle(), read_only_inheritance_schema().schema().handle());

    EXPECT_EQ(manifest.lookup("unknown"), nullptr);
}

TEST(AuthorizationManifestTest, EmptyDocuments) {
    AuthorizationManifest manifest;
    EXPECT_FALSE(manifest.is_registered("pk"));
    EXPECT_FALSE(manifest.full_access("pk"));
    EXPECT_EQ(manifest.lookup("pk"), nullptr);
}

TEST(AuthorizationManifestTest, IsAuthorized) {
    string link_handle;
    auto db = db_with_inheritance_link(&link_handle);

    AuthorizationSchema schema = read_only_inheritance_schema();
    AuthorizationManifest manifest;
    manifest.add_document(make_document("pk", false, {schema}));

    auto link = db->get_link(link_handle);
    ASSERT_NE(link, nullptr);

    EXPECT_FALSE(manifest.is_authorized(*link, "unknown", AuthorizationOperation::READ, *db));
    EXPECT_FALSE(manifest.is_authorized(link_handle, "unknown", AuthorizationOperation::READ, *db));

    EXPECT_TRUE(manifest.is_authorized(*link, "pk", AuthorizationOperation::READ, *db));
    EXPECT_TRUE(manifest.is_authorized(link_handle, "pk", AuthorizationOperation::READ, *db));

    EXPECT_FALSE(manifest.is_authorized(*link, "pk", AuthorizationOperation::WRITE, *db));
    EXPECT_FALSE(manifest.is_authorized(link_handle, "pk", AuthorizationOperation::WRITE, *db));
}

TEST(AuthorizationManifestTest, FullAccessGrantsAllOperations) {
    string link_handle;
    auto db = db_with_inheritance_link(&link_handle);

    AuthorizationManifest manifest;
    manifest.add_document(make_document("pk", true, {}));

    auto link = db->get_link(link_handle);
    ASSERT_NE(link, nullptr);

    EXPECT_TRUE(manifest.is_authorized(*link, "pk", AuthorizationOperation::READ, *db));
    EXPECT_TRUE(manifest.is_authorized(*link, "pk", AuthorizationOperation::WRITE, *db));
    EXPECT_TRUE(manifest.is_authorized(link_handle, "pk", AuthorizationOperation::WRITE, *db));
}

TEST(AuthorizationProfileTest, FromDocumentWithAndWithoutSchema) {
    auto document = make_document("pk", false, {read_only_inheritance_schema()});
    auto profile = AuthorizationProfile::from_document(*document);

    EXPECT_EQ(profile.access_key(), "pk");
    EXPECT_FALSE(profile.is_full_access());
    ASSERT_EQ(profile.schemas().size(), 1u);
    EXPECT_TRUE(profile.schemas()[0].read());
    EXPECT_FALSE(profile.schemas()[0].write());

    AuthorizationSchema read_write(inheritance_mammal_tokens(), true, true);
    auto replaced = profile.with_schema(read_write);
    ASSERT_EQ(replaced.schemas().size(), 1u);
    EXPECT_TRUE(replaced.schemas()[0].write());

    auto extra_tokens = inheritance_mammal_tokens();
    extra_tokens.back() = "\"animal\"";
    AuthorizationSchema extra(extra_tokens, true, false);
    auto appended = replaced.with_schema(extra);
    ASSERT_EQ(appended.schemas().size(), 2u);

    auto after_one_removed = appended.without_schema(extra);
    ASSERT_TRUE(after_one_removed.has_value());
    EXPECT_EQ(after_one_removed->schemas().size(), 1u);

    auto after_last_removed = after_one_removed->without_schema(read_write);
    EXPECT_FALSE(after_last_removed.has_value());
}

TEST(AuthorizationProfileTest, FullAccessRejectsSchemas) {
    auto document = make_document("pk", true, {});
    auto profile = AuthorizationProfile::from_document(*document);
    EXPECT_TRUE(profile.is_full_access());
    EXPECT_TRUE(profile.schemas().empty());
    EXPECT_THROW(profile.with_schema(read_only_inheritance_schema()), runtime_error);
}

TEST(AuthorizationManagerTest, ManifestReflectsPersistedPermissions) {
    string link_handle;
    auto db = db_with_inheritance_link(&link_handle);

    auto persistence = make_shared<DummyPersistence>();
    AuthorizationManager manager(persistence);
    AuthorizationSchema schema = read_only_inheritance_schema();

    manager.authorize("pk", schema);

    AuthorizationManifest manifest = manifest_from_persistence(*persistence);
    auto link = db->get_link(link_handle);
    ASSERT_NE(link, nullptr);

    EXPECT_TRUE(manifest.is_authorized(*link, "pk", AuthorizationOperation::READ, *db));
    EXPECT_FALSE(manifest.is_authorized(*link, "pk", AuthorizationOperation::WRITE, *db));
}

TEST(AuthorizationManagerTest, AuthorizeThenReadAndWriteFlags) {
    auto persistence = make_shared<DummyPersistence>();
    AuthorizationManager manager(persistence);
    AuthorizationSchema schema = read_only_inheritance_schema();

    manager.authorize("pk", schema);
    manager.authorize("pk2", schema);

    EXPECT_EQ(persistence->documents.size(), 2u);
    ASSERT_EQ(persistence->documents["pk"].size(), 1u);
    ASSERT_EQ(persistence->documents["pk2"].size(), 1u);
    EXPECT_EQ(persistence->documents["pk"][0].schema().handle(), schema.schema().handle());
    EXPECT_EQ(persistence->documents["pk2"][0].schema().handle(), schema.schema().handle());

    const auto entries = manager.list("pk");
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_TRUE(entries[0].read());
    EXPECT_FALSE(entries[0].write());
    EXPECT_TRUE(manager.list("unknown").empty());

    manager.revoke("pk", schema);
    EXPECT_EQ(persistence->documents.size(), 2u);
    EXPECT_TRUE(persistence->documents["pk"].empty());
    EXPECT_EQ(persistence->documents["pk2"].size(), 1u);

    manager.revoke_all("pk");
    EXPECT_EQ(persistence->documents.size(), 1u);
    EXPECT_EQ(persistence->documents.count("pk"), 0u);
}
