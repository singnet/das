#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "AuthorizationManagement.h"
#include "AuthorizationManifest.h"
#include "AuthorizationPersistence.h"
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

AccessPermissionEntry read_only_inheritance_entry() {
    return AccessPermissionEntry(inheritance_mammal_tokens(), true, false);
}

class FakePersistence : public AuthorizationPersistence {
   public:
    int save_count = 0;
    int remove_count = 0;
    int remove_all_count = 0;
    string last_key;
    string last_handle;

    void save(const string& public_key, const AccessPermissionEntry& entry) override {
        this->save_count++;
        this->last_key = public_key;
        this->last_handle = entry.schema.handle();
    }

    void remove(const string& public_key, const string& handle) override {
        this->remove_count++;
        this->last_key = public_key;
        this->last_handle = handle;
    }

    void remove_all(const string& public_key) override {
        this->remove_all_count++;
        this->last_key = public_key;
    }
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

TEST(AuthorizationManifestTest, SetAddRemoveAndFullAccess) {
    AuthorizationManifest manifest;
    string key = "pk1";
    EXPECT_FALSE(manifest.is_registered(key));
    EXPECT_FALSE(manifest.full_access(key));
    EXPECT_TRUE(manifest.entries(key).empty());

    AccessPermissionEntry entry = read_only_inheritance_entry();
    manifest.add(key, entry);
    ASSERT_TRUE(manifest.is_registered(key));
    ASSERT_EQ(manifest.entries(key).size(), 1u);
    EXPECT_TRUE(manifest.entries(key)[0].read);
    EXPECT_FALSE(manifest.entries(key)[0].write);
    EXPECT_EQ(manifest.entries(key)[0].schema.handle(), entry.schema.handle());

    manifest.remove(key, entry.schema.handle());
    EXPECT_TRUE(manifest.is_registered(key));
    EXPECT_TRUE(manifest.entries(key).empty());

    manifest.set(AccessPermissionDocument(key, true, {}));
    EXPECT_TRUE(manifest.full_access(key));

    manifest.remove_all(key);
    EXPECT_FALSE(manifest.is_registered(key));
}

TEST(AuthorizationManifestTest, AddReplacesEntryWithSameSchemaHandle) {
    AuthorizationManifest manifest;
    string key = "pk1";
    manifest.add(key, AccessPermissionEntry(inheritance_mammal_tokens(), true, false));
    manifest.add(key, AccessPermissionEntry(inheritance_mammal_tokens(), false, true));

    ASSERT_EQ(manifest.entries(key).size(), 1u);
    EXPECT_FALSE(manifest.entries(key)[0].read);
    EXPECT_TRUE(manifest.entries(key)[0].write);
}

TEST(AuthorizationManagementTest, RejectsNullAtomDB) {
    EXPECT_THROW(AuthorizationManagement(nullptr, nullptr), runtime_error);
}

TEST(AuthorizationManagementTest, AdministrationRequiresPersistence) {
    auto db = make_shared<InMemoryDB>("auth_admin_");
    AuthorizationManagement management(db, nullptr);
    AccessPermissionEntry entry = read_only_inheritance_entry();
    EXPECT_THROW(management.authorize("pk", entry), runtime_error);
    EXPECT_THROW(management.revoke("pk", entry.schema.handle()), runtime_error);
    EXPECT_THROW(management.revoke_all("pk"), runtime_error);
}

TEST(AuthorizationManagementTest, UnregisteredKeyIsDenied) {
    string link_handle;
    auto db = db_with_inheritance_link(&link_handle);
    auto persistence = make_shared<FakePersistence>();
    AuthorizationManagement management(db, persistence);

    auto link = db->get_link(link_handle);
    ASSERT_NE(link, nullptr);
    EXPECT_FALSE(management.is_authorized(*link, "unknown", AuthorizationOperation::READ));
    EXPECT_FALSE(management.is_authorized(link_handle, "unknown", AuthorizationOperation::READ, *db));
}

TEST(AuthorizationManagementTest, AuthorizeThenReadAndWriteFlags) {
    string link_handle;
    auto db = db_with_inheritance_link(&link_handle);
    auto persistence = make_shared<FakePersistence>();
    AuthorizationManagement management(db, persistence);
    AccessPermissionEntry entry = read_only_inheritance_entry();

    management.authorize("pk", entry);
    EXPECT_EQ(persistence->save_count, 1);
    EXPECT_EQ(persistence->last_key, "pk");

    auto link = db->get_link(link_handle);
    ASSERT_NE(link, nullptr);
    EXPECT_TRUE(management.is_authorized(*link, "pk", AuthorizationOperation::READ));
    EXPECT_FALSE(management.is_authorized(*link, "pk", AuthorizationOperation::WRITE));
    EXPECT_TRUE(management.is_authorized(link_handle, "pk", AuthorizationOperation::READ, *db));
    EXPECT_FALSE(management.is_authorized(link_handle, "pk", AuthorizationOperation::WRITE, *db));
}

TEST(AuthorizationManagementTest, RevokeRemovesAccess) {
    string link_handle;
    auto db = db_with_inheritance_link(&link_handle);
    auto persistence = make_shared<FakePersistence>();
    AuthorizationManagement management(db, persistence);
    AccessPermissionEntry entry = read_only_inheritance_entry();

    management.authorize("pk", entry);
    management.revoke("pk", entry.schema.handle());
    EXPECT_EQ(persistence->remove_count, 1);

    auto link = db->get_link(link_handle);
    ASSERT_NE(link, nullptr);
    EXPECT_FALSE(management.is_authorized(*link, "pk", AuthorizationOperation::READ));
}

TEST(AuthorizationManagementTest, DoesNotLoadPermissionsFromAtomDB) {
    class PermissionsInMemoryDB : public InMemoryDB {
       public:
        explicit PermissionsInMemoryDB(const string& context) : InMemoryDB(context) {}
        vector<AccessPermissionDocument> get_access_permissions(
            const PublicKey& public_key) const override {
            return {AccessPermissionDocument("pk", true, {})};
        }
    };

    auto db = make_shared<PermissionsInMemoryDB>("auth_noload_");
    auto persistence = make_shared<FakePersistence>();
    AuthorizationManagement management(persistence);
}
