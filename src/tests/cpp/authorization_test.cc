#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "AuthorizationManager.h"
#include "AuthorizationManifest.h"
#include "AuthorizationPersistence.h"
#include "InMemoryDB.h"
#include "Link.h"
#include "LinkSchema.h"
#include "ManifestAuthorizer.h"
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

AccessPermissionEntry read_only_inheritance_entry() {
    return AccessPermissionEntry(inheritance_mammal_tokens(), true, false);
}

class DummyPersistence : public AuthorizationPersistence {
   public:
    map<string, vector<string>> documents;

    vector<atomdb_api_types::AccessPermissionEntry> list(const string& public_key) override {
        return {};
    }

    void save(const string& public_key, const atomdb_api_types::AccessPermissionEntry& entry) override {
        auto& handles = documents[public_key];
        handles.push_back(entry.schema.handle());
    }

    void remove(const string& public_key,
                const atomdb_api_types::AccessPermissionEntry& entry) override {
        auto it = documents.find(public_key);
        if (it == documents.end()) {
            return;
        }

        auto& handles = it->second;

        auto handle = entry.schema.handle();

        handles.erase(std::remove(handles.begin(), handles.end(), handle), handles.end());
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

}  // namespace

TEST(AuthorizationManifestTest, ManagesAuthorizationEntriesAndAccess) {
    AuthorizationManifest manifest;
    string public_key = "pk1";

    EXPECT_FALSE(manifest.is_registered(public_key));
    EXPECT_FALSE(manifest.full_access(public_key));

    EXPECT_EQ(manifest.get_document(public_key), nullptr);

    AccessPermissionEntry entry = read_only_inheritance_entry();
    manifest.add(public_key, entry);

    ASSERT_TRUE(manifest.is_registered(public_key));

    auto doc = manifest.get_document(public_key);
    ASSERT_NE(doc, nullptr);

    auto entries = doc->entries;
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_TRUE(entries[0].read);
    EXPECT_FALSE(entries[0].write);
    EXPECT_EQ(entries[0].schema.handle(), entry.schema.handle());

    manifest.remove(public_key, entry);
    EXPECT_TRUE(manifest.is_registered(public_key));

    EXPECT_TRUE(manifest.get_document(public_key)->entries.empty());

    manifest.set(AccessPermissionDocument(public_key, true, {}));
    EXPECT_TRUE(manifest.full_access(public_key));

    manifest.remove_all(public_key);
    EXPECT_FALSE(manifest.is_registered(public_key));
}

TEST(AuthorizationManifestTest, AddReplacesEntryWithSameSchemaHandle) {
    AuthorizationManifest manifest;
    string public_key = "pk1";
    manifest.add(public_key, AccessPermissionEntry(inheritance_mammal_tokens(), true, false));
    manifest.add(public_key, AccessPermissionEntry(inheritance_mammal_tokens(), false, true));

    auto entries = manifest.get_document(public_key)->entries;

    ASSERT_EQ(entries.size(), 1u);
    EXPECT_FALSE(entries[0].read);
    EXPECT_TRUE(entries[0].write);
}

TEST(ManifestAuthorizerTest, IsAuthorized) {
    string link_handle;
    auto db = db_with_inheritance_link(&link_handle);

    auto manifest = make_shared<AuthorizationManifest>();
    auto authorizer = make_shared<ManifestAuthorizer>(manifest);

    auto link = db->get_link(link_handle);
    ASSERT_NE(link, nullptr);
    EXPECT_FALSE(authorizer->is_authorized(*link, "unknown", AuthorizationOperation::READ, *db));
    EXPECT_FALSE(authorizer->is_authorized(link_handle, "unknown", AuthorizationOperation::READ, *db));

    AccessPermissionEntry entry = read_only_inheritance_entry();
    manifest->add("pk", entry);
    EXPECT_TRUE(authorizer->is_authorized(*link, "pk", AuthorizationOperation::READ, *db));
    EXPECT_TRUE(authorizer->is_authorized(link_handle, "pk", AuthorizationOperation::READ, *db));

    EXPECT_FALSE(authorizer->is_authorized(*link, "pk", AuthorizationOperation::WRITE, *db));
    EXPECT_FALSE(authorizer->is_authorized(link_handle, "pk", AuthorizationOperation::WRITE, *db));
}

TEST(AuthorizationManagerTest, AuthorizeThenReadAndWriteFlags) {
    auto persistence = make_shared<DummyPersistence>();
    AuthorizationManager manager(persistence);

    AccessPermissionEntry read_entry = read_only_inheritance_entry();
    EXPECT_TRUE(read_entry.read);
    EXPECT_FALSE(read_entry.write);

    manager.authorize("pk", read_entry);
    manager.authorize("pk2", read_entry);
    EXPECT_EQ(persistence->documents.size(), 2u);
    EXPECT_EQ(persistence->documents["pk"], vector<string>{read_entry.schema.handle()});
    EXPECT_EQ(persistence->documents["pk2"], vector<string>{read_entry.schema.handle()});

    manager.revoke("pk", read_entry);
    EXPECT_EQ(persistence->documents.size(), 2u);
    EXPECT_EQ(persistence->documents["pk"], vector<string>{});
    EXPECT_EQ(persistence->documents["pk2"], vector<string>{read_entry.schema.handle()});

    AccessPermissionEntry write_entry(inheritance_mammal_tokens(), false, true);
    EXPECT_FALSE(write_entry.read);
    EXPECT_TRUE(write_entry.write);

    manager.authorize("pk_write", write_entry);
    EXPECT_EQ(persistence->documents["pk_write"], vector<string>{write_entry.schema.handle()});

    manager.revoke("pk_write", write_entry);
    EXPECT_EQ(persistence->documents["pk_write"], vector<string>{});

    AccessPermissionEntry invalid_permission_entry(inheritance_mammal_tokens(), false, false);
    EXPECT_FALSE(invalid_permission_entry.read);
    EXPECT_FALSE(invalid_permission_entry.write);

    manager.authorize("pk_invalid_permission", invalid_permission_entry);
    EXPECT_EQ(persistence->documents["pk_invalid_permission"],
              vector<string>{invalid_permission_entry.schema.handle()});

    manager.authorize("", read_entry);
    EXPECT_EQ(persistence->documents[""], vector<string>{read_entry.schema.handle()});

    vector<string> whitespace_only_tokens;
    EXPECT_TRUE(whitespace_only_tokens.empty());

    manager.revoke("never_authorized", read_entry);
    EXPECT_EQ(persistence->documents.find("never_authorized"), persistence->documents.end());

    manager.revoke_all("pk");
    EXPECT_EQ(persistence->documents.find("pk"), persistence->documents.end());
    EXPECT_EQ(persistence->documents["pk2"], vector<string>{read_entry.schema.handle()});
}