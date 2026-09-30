#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "InMemoryAccessPermissionTypes.h"
#include "InMemoryDB.h"
#include "Keychain.h"
#include "Link.h"
#include "LinkSchema.h"
#include "Node.h"
#include "ProtectedAtomDB.h"
#include "RemoteAtomDB.h"
#include "RemoteAtomDBPeer.h"
#include "TestAtomDBJsonConfig.h"

using namespace atomdb;
using namespace atomdb_api_types;
using namespace atoms;
using namespace std;

namespace {

const string similarity_database_uid = "similarity_database";
const string full_access_database_uid = "full_access_database";
const string similarity_public_key = "similarity_human_reader";
const string full_access_public_key = "full_access_reader";

set<string> handles_from_handle_set(const shared_ptr<HandleSet>& handle_set) {
    set<string> handles;
    if (handle_set == nullptr) return handles;
    auto it = handle_set->get_iterator();
    while (char* handle = it->next()) handles.insert(handle);
    return handles;
}

// In-memory AtomDB that ProtectedAtomDB can authorize against, without Redis or Mongo.
class InMemoryDBWithAccessDocuments : public InMemoryDB {
   public:
    explicit InMemoryDBWithAccessDocuments(const string& database_uid)
        : InMemoryDB(test_atomdb_json_config("inmemorydb", "", database_uid)) {}

    ProtectionMode get_protection_mode() const override { return ProtectionMode::PROTECTED; }

    void grant_full_access(const string& public_key) {
        auto document = make_shared<InMemoryAccessPermissionDocument>();
        document->set_access_key(public_key);
        document->set_full_access(true);
        this->documents[public_key] = document;
    }

    void grant_link_template(const string& public_key, const vector<string>& tokens) {
        auto document = make_shared<InMemoryAccessPermissionDocument>();
        document->set_access_key(public_key);
        document->set_full_access(false);
        document->append_entry(tokens, true, false);
        this->documents[public_key] = document;
    }

    shared_ptr<AccessPermissionDocument> get_access_permissions(
        const string& public_key) const override {
        auto it = this->documents.find(public_key);
        if (it == this->documents.end()) return nullptr;
        return it->second;
    }

   private:
    map<string, shared_ptr<InMemoryAccessPermissionDocument>> documents;
};

// Three peers:
// - similarity_database: protected, may read only Similarity "human" *
// - full_access_database: protected, may read every atom
// - unprotected peer: no keychain, every atom is visible
//
class RemoteAtomDBKeySensitiveTest : public ::testing::Test {
   protected:
    string similarity_handle = Node("Symbol", "Similarity").handle();
    string inheritance_handle = Node("Symbol", "Inheritance").handle();
    string human_handle = Node("Symbol", "\"human\"").handle();
    string monkey_handle = Node("Symbol", "\"monkey\"").handle();
    string chimp_handle = Node("Symbol", "\"chimp\"").handle();
    string snake_handle = Node("Symbol", "\"snake\"").handle();
    string vine_handle = Node("Symbol", "\"vine\"").handle();
    string mammal_handle = Node("Symbol", "\"mammal\"").handle();

    string similarity_human_monkey_handle =
        Link("Expression", {similarity_handle, human_handle, monkey_handle}).handle();
    string similarity_human_chimp_handle =
        Link("Expression", {similarity_handle, human_handle, chimp_handle}).handle();
    string similarity_snake_vine_handle =
        Link("Expression", {similarity_handle, snake_handle, vine_handle}).handle();
    string inheritance_human_mammal_handle =
        Link("Expression", {inheritance_handle, human_handle, mammal_handle}).handle();

    vector<string> similarity_human_v_tokens = {"LINK_TEMPLATE",
                                                "Expression",
                                                "3",
                                                "NODE",
                                                "Symbol",
                                                "Similarity",
                                                "NODE",
                                                "Symbol",
                                                "\"human\"",
                                                "VARIABLE",
                                                "V"};
    vector<string> inheritance_human_v_tokens = {"LINK_TEMPLATE",
                                                 "Expression",
                                                 "3",
                                                 "NODE",
                                                 "Symbol",
                                                 "Inheritance",
                                                 "NODE",
                                                 "Symbol",
                                                 "\"human\"",
                                                 "VARIABLE",
                                                 "V"};

    LinkSchema similarity_human_schema{this->similarity_human_v_tokens};
    LinkSchema inheritance_human_schema{this->inheritance_human_v_tokens};

    shared_ptr<RemoteAtomDB> db;

    void SetUp() override {
        Node similarity_node("Symbol", "Similarity");
        Node human_node("Symbol", "\"human\"");
        Node monkey_node("Symbol", "\"monkey\"");
        Node chimp_node("Symbol", "\"chimp\"");
        Node snake_node("Symbol", "\"snake\"");
        Node vine_node("Symbol", "\"vine\"");
        Node inheritance_node("Symbol", "Inheritance");
        Node mammal_node("Symbol", "\"mammal\"");
        Link similarity_human_monkey_link(
            "Expression", {this->similarity_handle, this->human_handle, this->monkey_handle});
        Link similarity_snake_vine_link(
            "Expression", {this->similarity_handle, this->snake_handle, this->vine_handle});
        Link inheritance_human_mammal_link(
            "Expression", {this->inheritance_handle, this->human_handle, this->mammal_handle});
        Link similarity_human_chimp_link(
            "Expression", {this->similarity_handle, this->human_handle, this->chimp_handle});

        auto similarity_backend = make_shared<InMemoryDBWithAccessDocuments>(similarity_database_uid);
        similarity_backend->grant_link_template(similarity_public_key, this->similarity_human_v_tokens);
        similarity_backend->add_node(&similarity_node);
        similarity_backend->add_node(&human_node);
        similarity_backend->add_node(&monkey_node);
        similarity_backend->add_node(&snake_node);
        similarity_backend->add_node(&vine_node);
        similarity_backend->add_link(&similarity_human_monkey_link);
        similarity_backend->add_link(&similarity_snake_vine_link);

        auto full_access_backend = make_shared<InMemoryDBWithAccessDocuments>(full_access_database_uid);
        full_access_backend->grant_full_access(full_access_public_key);
        full_access_backend->add_node(&human_node);
        full_access_backend->add_node(&inheritance_node);
        full_access_backend->add_node(&mammal_node);
        full_access_backend->add_link(&inheritance_human_mammal_link);

        auto unprotected_backend =
            make_shared<InMemoryDB>(test_atomdb_json_config("inmemorydb", "", "unprotected_database"));
        unprotected_backend->add_node(&similarity_node);
        unprotected_backend->add_node(&human_node);
        unprotected_backend->add_node(&chimp_node);
        unprotected_backend->add_node(&snake_node);
        unprotected_backend->add_node(&vine_node);
        unprotected_backend->add_link(&similarity_human_chimp_link);
        unprotected_backend->add_link(&similarity_snake_vine_link);

        map<string, shared_ptr<RemoteAtomDBPeer>> peers;
        peers["similarity_peer"] = make_shared<RemoteAtomDBPeer>(
            "similarity_peer", make_shared<ProtectedAtomDB>(similarity_backend), nullptr);
        peers["full_access_peer"] = make_shared<RemoteAtomDBPeer>(
            "full_access_peer", make_shared<ProtectedAtomDB>(full_access_backend), nullptr);
        peers["unprotected_peer"] =
            make_shared<RemoteAtomDBPeer>("unprotected_peer", unprotected_backend, nullptr);

        this->db = make_shared<RemoteAtomDB>("remote", peers);
    }

    shared_ptr<Keychain> keychain(const map<string, string>& database_uid_to_public_key) const {
        return make_shared<Keychain>(database_uid_to_public_key);
    }

    shared_ptr<Keychain> similarity_keychain() const {
        return this->keychain({{similarity_database_uid, similarity_public_key}});
    }

    shared_ptr<Keychain> full_access_keychain() const {
        return this->keychain({{full_access_database_uid, full_access_public_key}});
    }

    shared_ptr<Keychain> both_keychains() const {
        return this->keychain({{similarity_database_uid, similarity_public_key},
                               {full_access_database_uid, full_access_public_key}});
    }
};

}  // namespace

TEST_F(RemoteAtomDBKeySensitiveTest, NullptrKeychainReadsOnlyTheUnprotectedPeer) {
    EXPECT_EQ(this->db->get_atom(this->similarity_human_monkey_handle, nullptr), nullptr);
    EXPECT_EQ(this->db->get_link(this->inheritance_human_mammal_handle, nullptr), nullptr);
    EXPECT_EQ(this->db->get_link(this->human_handle, nullptr), nullptr);

    EXPECT_EQ(this->db->get_link(this->similarity_human_chimp_handle, nullptr)->handle(),
              this->similarity_human_chimp_handle);
    EXPECT_EQ(this->db->get_node(this->human_handle, nullptr)->handle(), this->human_handle);

    EXPECT_EQ(
        handles_from_handle_set(this->db->query_for_pattern(this->similarity_human_schema, nullptr)),
        set<string>({this->similarity_human_chimp_handle}));
}

TEST_F(RemoteAtomDBKeySensitiveTest, SimilarityKeyDoesNotOpenTheFullAccessDatabase) {
    auto keychain = this->similarity_keychain();

    auto granted = this->db->get_link(this->similarity_human_monkey_handle, keychain);
    ASSERT_NE(granted, nullptr);
    EXPECT_EQ(granted->handle(), this->similarity_human_monkey_handle);

    EXPECT_EQ(this->db->get_atom(this->inheritance_human_mammal_handle, keychain), nullptr);
    EXPECT_EQ(this->db->get_link(this->inheritance_human_mammal_handle, keychain), nullptr);
    EXPECT_EQ(this->db->get_node(this->mammal_handle, keychain), nullptr);
    EXPECT_EQ(this->db->get_link(this->similarity_snake_vine_handle, keychain)->handle(),
              this->similarity_snake_vine_handle);
    EXPECT_EQ(this->db->get_node(this->human_handle, keychain)->handle(), this->human_handle);

    EXPECT_EQ(
        handles_from_handle_set(this->db->query_for_pattern(this->similarity_human_schema, keychain)),
        set<string>({this->similarity_human_monkey_handle, this->similarity_human_chimp_handle}));
    EXPECT_TRUE(
        handles_from_handle_set(this->db->query_for_pattern(this->inheritance_human_schema, keychain))
            .empty());
}

TEST_F(RemoteAtomDBKeySensitiveTest, FullAccessKeyDoesNotOpenTheSimilarityDatabase) {
    auto keychain = this->full_access_keychain();

    EXPECT_EQ(this->db->get_atom(this->similarity_human_monkey_handle, keychain), nullptr);
    EXPECT_EQ(this->db->get_link(this->inheritance_human_mammal_handle, keychain)->handle(),
              this->inheritance_human_mammal_handle);
    EXPECT_EQ(this->db->get_node(this->mammal_handle, keychain)->handle(), this->mammal_handle);
    EXPECT_EQ(this->db->get_link(this->similarity_snake_vine_handle, keychain)->handle(),
              this->similarity_snake_vine_handle);

    EXPECT_EQ(
        handles_from_handle_set(this->db->query_for_pattern(this->similarity_human_schema, keychain)),
        set<string>({this->similarity_human_chimp_handle}));
    EXPECT_EQ(
        handles_from_handle_set(this->db->query_for_pattern(this->inheritance_human_schema, keychain)),
        set<string>({this->inheritance_human_mammal_handle}));
}

TEST_F(RemoteAtomDBKeySensitiveTest, BothKeysUnionEachDatabaseWithTheUnprotectedPeer) {
    auto keychain = this->both_keychains();

    EXPECT_EQ(this->db->get_link(this->similarity_human_monkey_handle, keychain)->handle(),
              this->similarity_human_monkey_handle);
    EXPECT_EQ(this->db->get_link(this->inheritance_human_mammal_handle, keychain)->handle(),
              this->inheritance_human_mammal_handle);
    EXPECT_EQ(this->db->get_node(this->mammal_handle, keychain)->handle(), this->mammal_handle);
    EXPECT_EQ(this->db->get_link(this->mammal_handle, keychain), nullptr);
    EXPECT_EQ(this->db->get_node(this->inheritance_human_mammal_handle, keychain), nullptr);

    EXPECT_EQ(
        handles_from_handle_set(this->db->query_for_pattern(this->similarity_human_schema, keychain)),
        set<string>({this->similarity_human_monkey_handle, this->similarity_human_chimp_handle}));
    EXPECT_EQ(
        handles_from_handle_set(this->db->query_for_pattern(this->inheritance_human_schema, keychain)),
        set<string>({this->inheritance_human_mammal_handle}));
}

TEST_F(RemoteAtomDBKeySensitiveTest, UnknownEmptyAndForeignKeysDoNotReadProtectedDatabases) {
    auto unknown_key =
        this->keychain({{similarity_database_uid, "unknown_reader"}, {full_access_database_uid, ""}});
    auto key_for_another_database = this->keychain({{"another_database", similarity_public_key}});

    EXPECT_EQ(this->db->get_atom(this->similarity_human_monkey_handle, unknown_key), nullptr);
    EXPECT_EQ(this->db->get_atom(this->inheritance_human_mammal_handle, key_for_another_database),
              nullptr);
    EXPECT_EQ(this->db->get_atom("ffffffffffffffffffffffffffffffff", this->both_keychains()), nullptr);
    EXPECT_EQ(this->db->get_link(this->similarity_human_chimp_handle, unknown_key)->handle(),
              this->similarity_human_chimp_handle);

    EXPECT_EQ(
        handles_from_handle_set(this->db->query_for_pattern(this->similarity_human_schema, unknown_key)),
        set<string>({this->similarity_human_chimp_handle}));
}

TEST_F(RemoteAtomDBKeySensitiveTest, ReleaseCacheDoesNotReturnStaleKeychainReads) {
    auto keychain = this->similarity_keychain();

    EXPECT_EQ(this->db->get_link(this->similarity_human_monkey_handle, keychain)->handle(),
              this->similarity_human_monkey_handle);
    EXPECT_EQ(
        handles_from_handle_set(this->db->query_for_pattern(this->similarity_human_schema, keychain)),
        set<string>({this->similarity_human_monkey_handle, this->similarity_human_chimp_handle}));

    this->db->get_peer("similarity_peer")->release_cache();
    this->db->get_peer("full_access_peer")->release_cache();
    this->db->get_peer("unprotected_peer")->release_cache();

    EXPECT_EQ(this->db->get_atom(this->similarity_human_monkey_handle, nullptr), nullptr);
    EXPECT_EQ(this->db->get_atom(this->inheritance_human_mammal_handle, keychain), nullptr);
    EXPECT_EQ(this->db->get_link(this->similarity_human_monkey_handle, keychain)->handle(),
              this->similarity_human_monkey_handle);
    EXPECT_EQ(
        handles_from_handle_set(this->db->query_for_pattern(this->similarity_human_schema, nullptr)),
        set<string>({this->similarity_human_chimp_handle}));
    EXPECT_EQ(
        handles_from_handle_set(this->db->query_for_pattern(this->similarity_human_schema, keychain)),
        set<string>({this->similarity_human_monkey_handle, this->similarity_human_chimp_handle}));
}

TEST_F(RemoteAtomDBKeySensitiveTest, NestedSchemaDoesNotMatchWhenTheTargetLinkIsNotStored) {
    Node evaluation("Symbol", "Evaluation");
    Node predicate("Symbol", "Predicate");
    Node is_animal("Symbol", "\"is_animal\"");
    Node human("Symbol", "\"human\"");
    Link predicate_is_animal("Expression", {predicate.handle(), is_animal.handle()});
    Link evaluation_is_animal_human("Expression",
                                    {evaluation.handle(), predicate_is_animal.handle(), human.handle()});

    // (Evaluation (Predicate "is_animal") $C)
    vector<string> nested_link_tokens = {"LINK_TEMPLATE",
                                         "Expression",
                                         "3",
                                         "NODE",
                                         "Symbol",
                                         "Evaluation",
                                         "LINK",
                                         "Expression",
                                         "2",
                                         "NODE",
                                         "Symbol",
                                         "Predicate",
                                         "NODE",
                                         "Symbol",
                                         "\"is_animal\"",
                                         "VARIABLE",
                                         "C"};

    // (Evaluation (Predicate $P) $C)
    vector<string> nested_template_tokens = {"LINK_TEMPLATE",
                                             "Expression",
                                             "3",
                                             "NODE",
                                             "Symbol",
                                             "Evaluation",
                                             "LINK_TEMPLATE",
                                             "Expression",
                                             "2",
                                             "NODE",
                                             "Symbol",
                                             "Predicate",
                                             "VARIABLE",
                                             "P",
                                             "VARIABLE",
                                             "C"};
    LinkSchema nested_link_schema{nested_link_tokens};
    LinkSchema nested_template_schema{nested_template_tokens};

    auto nested_link_backend = make_shared<InMemoryDBWithAccessDocuments>("nested_link_database");
    nested_link_backend->grant_link_template("nested_reader", nested_link_tokens);
    nested_link_backend->add_link(&evaluation_is_animal_human);

    auto nested_template_backend =
        make_shared<InMemoryDBWithAccessDocuments>("nested_template_database");
    nested_template_backend->grant_link_template("nested_reader", nested_template_tokens);
    nested_template_backend->add_link(&evaluation_is_animal_human);

    map<string, shared_ptr<RemoteAtomDBPeer>> peers;
    peers["nested_link_peer"] = make_shared<RemoteAtomDBPeer>(
        "nested_link_peer", make_shared<ProtectedAtomDB>(nested_link_backend), nullptr);
    peers["nested_template_peer"] = make_shared<RemoteAtomDBPeer>(
        "nested_template_peer", make_shared<ProtectedAtomDB>(nested_template_backend), nullptr);
    auto db = make_shared<RemoteAtomDB>("remote_nested", peers);
    auto keys = this->keychain(
        {{"nested_link_database", "nested_reader"}, {"nested_template_database", "nested_reader"}});

    EXPECT_EQ(db->get_atom(evaluation_is_animal_human.handle(), keys), nullptr);
    EXPECT_TRUE(handles_from_handle_set(db->query_for_pattern(nested_link_schema, keys)).empty());
    EXPECT_TRUE(handles_from_handle_set(db->query_for_pattern(nested_template_schema, keys)).empty());
}

// First read loads H from the protected backend and stores it in the read cache.
// H is then deleted from the backend. The second read must still return H from the cache.
// With the cache uid left empty, the keychain does not match and this returns nullptr.
TEST_F(RemoteAtomDBKeySensitiveTest, SecondGetAtomReadsTheCachedAtomAfterTheBackendLosesIt) {
    Node similarity("Symbol", "Similarity");
    Node human("Symbol", "\"human\"");
    Node monkey("Symbol", "\"monkey\"");
    Link similarity_human_monkey("Expression", {similarity.handle(), human.handle(), monkey.handle()});
    vector<string> tokens = {"LINK_TEMPLATE",
                             "Expression",
                             "3",
                             "NODE",
                             "Symbol",
                             "Similarity",
                             "NODE",
                             "Symbol",
                             "\"human\"",
                             "VARIABLE",
                             "V"};

    auto remote_atomdb = make_shared<InMemoryDBWithAccessDocuments>("cache_database");
    remote_atomdb->grant_link_template("cache_reader", tokens);
    remote_atomdb->add_node(&similarity);
    remote_atomdb->add_node(&human);
    remote_atomdb->add_node(&monkey);
    remote_atomdb->add_link(&similarity_human_monkey);

    map<string, shared_ptr<RemoteAtomDBPeer>> peers;
    peers["cache_peer"] = make_shared<RemoteAtomDBPeer>(
        "cache_peer", make_shared<ProtectedAtomDB>(remote_atomdb), nullptr);

    auto db = make_shared<RemoteAtomDB>("remote_cache", peers);
    auto keys = this->keychain({{"cache_database", "cache_reader"}});

    auto first = db->get_atom(similarity_human_monkey.handle(), keys);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->handle(), similarity_human_monkey.handle());

    remote_atomdb->delete_atom(similarity_human_monkey.handle());

    auto second = db->get_atom(similarity_human_monkey.handle(), keys);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->handle(), similarity_human_monkey.handle());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
