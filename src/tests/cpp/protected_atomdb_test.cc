#include <gtest/gtest.h>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Hasher.h"
#include "InMemoryAccessPermissionTypes.h"
#include "InMemoryDB.h"
#include "Keychain.h"
#include "Link.h"
#include "LinkSchema.h"
#include "MockAnimalsData.h"
#include "MongodbAuthorizationPersistence.h"
#include "Node.h"
#include "ProtectedAtomDB.h"
#include "RedisMongoDB.h"
#include "TestAtomDBJsonConfig.h"

using namespace atomdb;
using namespace atomdb_api_types;
using namespace atoms;
using namespace commons;
using namespace std;

namespace {

constexpr const char* uid = "animals_db";
constexpr const char* PKAdmin = "pk_admin";
constexpr const char* PKSimilarityHuman = "pk_similarity_human";
constexpr const char* PKRelatedHuman = "pk_related_human";
constexpr const char* PKUnknown = "pk_unknown";
constexpr const char* PKOnlyH = "pk_only_h";
constexpr const char* PKOnlyABC = "pk_only_abc";

string symbol_handle(const string& name) { return Node("Symbol", name).handle(); }

string expression_handle(const vector<string>& targets) { return Link("Expression", targets).handle(); }

shared_ptr<Keychain> make_keychain(const string& db_uid, const string& public_key) {
    return make_shared<Keychain>(map<string, string>{{db_uid, public_key}});
}

set<string> handles_from_set(const shared_ptr<HandleSet>& handle_set) {
    set<string> handles;
    if (handle_set == nullptr) {
        return handles;
    }
    auto it = handle_set->get_iterator();
    while (true) {
        char* handle = it->next();
        if (handle == nullptr) {
            break;
        }
        handles.insert(handle);
    }
    return handles;
}

vector<string> handles_from_list(const shared_ptr<HandleList>& handle_list) {
    vector<string> handles;
    if (handle_list == nullptr) {
        return handles;
    }
    for (unsigned int i = 0; i < handle_list->size(); ++i) {
        const char* handle = handle_list->get_handle(i);
        if (handle != nullptr) {
            handles.push_back(handle);
        }
    }
    return handles;
}

vector<string> similarity_human_tokens() {
    return {"LINK_TEMPLATE",
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
}

struct Animals {
    string similarity = symbol_handle("Similarity");
    string inheritance = symbol_handle("Inheritance");
    string related = symbol_handle("Related");
    string human = symbol_handle("\"human\"");
    string monkey = symbol_handle("\"monkey\"");
    string chimp = symbol_handle("\"chimp\"");
    string ent = symbol_handle("\"ent\"");
    string mammal = symbol_handle("\"mammal\"");
    string snake = symbol_handle("\"snake\"");
    string earthworm = symbol_handle("\"earthworm\"");
    string vine = symbol_handle("\"vine\"");

    string similarity_human_monkey = expression_handle({similarity, human, monkey});
    string similarity_human_chimp = expression_handle({similarity, human, chimp});
    string similarity_chimp_monkey = expression_handle({similarity, chimp, monkey});
    string similarity_snake_earthworm = expression_handle({similarity, snake, earthworm});
    string similarity_snake_vine = expression_handle({similarity, snake, vine});
    string similarity_human_ent = expression_handle({similarity, human, ent});
    string similarity_monkey_human = expression_handle({similarity, monkey, human});
    string similarity_chimp_human = expression_handle({similarity, chimp, human});
    string similarity_monkey_chimp = expression_handle({similarity, monkey, chimp});
    string similarity_ent_human = expression_handle({similarity, ent, human});

    string inheritance_human_mammal = expression_handle({inheritance, human, mammal});
    string inheritance_monkey_mammal = expression_handle({inheritance, monkey, mammal});

    string related_similarity_human_monkey_similarity_human_chimp =
        expression_handle({related, similarity_human_monkey, similarity_human_chimp});
    string related_similarity_human_monkey_similarity_chimp_monkey =
        expression_handle({related, similarity_human_monkey, similarity_chimp_monkey});
    string related_similarity_human_monkey_similarity_human_ent =
        expression_handle({related, similarity_human_monkey, similarity_human_ent});
    string related_similarity_human_monkey_similarity_monkey_human =
        expression_handle({related, similarity_human_monkey, similarity_monkey_human});
    string related_similarity_human_monkey_similarity_chimp_human =
        expression_handle({related, similarity_human_monkey, similarity_chimp_human});
    string related_similarity_human_monkey_similarity_monkey_chimp =
        expression_handle({related, similarity_human_monkey, similarity_monkey_chimp});
    string related_similarity_human_monkey_similarity_ent_human =
        expression_handle({related, similarity_human_monkey, similarity_ent_human});
    string related_similarity_and_inheritance =
        expression_handle({related, similarity_human_monkey, inheritance_human_mammal});
    string related_similarity_human_monkey_inheritance_monkey_mammal =
        expression_handle({related, similarity_human_monkey, inheritance_monkey_mammal});
};

class TestRedisMongoDB : public RedisMongoDB {
   public:
    explicit TestRedisMongoDB(const JsonConfig& config) : RedisMongoDB(config) {}
};

struct ProtectedRedisMongo {
    JsonConfig config;
    shared_ptr<TestRedisMongoDB> backend;
    shared_ptr<MongodbAuthorizationPersistence> persistence;
    shared_ptr<ProtectedAtomDB> db;

    explicit ProtectedRedisMongo(bool load_animals) {
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::make_document;

        this->config = test_atomdb_json_config("redismongodb", "protected_atomdb_test_", uid);

        TestRedisMongoDB seed(this->config);
        seed.drop_all();
        auto conn = seed.get_mongo_pool()->acquire();
        auto collection = (*conn)[seed.MONGODB_DB_NAME][seed.MONGODB_CONFIG_COLLECTION_NAME];
        collection.delete_many({});
        collection.insert_one(
            make_document(kvp("_id", Hasher::plain_string_hash(seed.MONGODB_CONFIG_COLLECTION_NAME)),
                          kvp("protected", true)));
        if (load_animals) {
            load_animals_related_data(seed);
        }

        this->backend = make_shared<TestRedisMongoDB>(this->config);
        this->persistence = make_shared<MongodbAuthorizationPersistence>(
            this->config.at_path("mongodb.endpoint").get_or<string>("localhost:40021"),
            this->config.at_path("mongodb.username").get_or<string>("admin"),
            this->config.at_path("mongodb.password").get_or<string>("admin"),
            this->backend->MONGODB_DB_NAME,
            this->backend->MONGODB_ACCESS_PERMISSIONS_COLLECTION_NAME);
        this->db = make_shared<ProtectedAtomDB>(this->backend);
    }

    ~ProtectedRedisMongo() { this->backend->drop_all(); }

    shared_ptr<Keychain> keys(const string& public_key) const {
        return make_keychain(this->db->get_uid(), public_key);
    }

    void grant_full_access(const string& public_key) {
        this->persistence->revoke(public_key);
        this->persistence->grant_unrestricted(public_key);
    }

    void grant_link_template(const string& public_key, const vector<string>& tokens) {
        this->persistence->revoke(public_key);
        vector<pair<LinkSchema, unsigned int>> schemas;
        schemas.push_back({LinkSchema(tokens), 1});
        this->persistence->grant(public_key, schemas);
    }
};

class AccessDocumentBackend : public InMemoryDB {
   public:
    explicit AccessDocumentBackend(string document_key)
        : InMemoryDB(test_atomdb_json_config("inmemorydb", "", "mem_db")),
          document_key(std::move(document_key)) {}

    shared_ptr<AccessPermissionDocument> get_access_permissions(const string&) const override {
        auto document = make_shared<InMemoryAccessPermissionDocument>();
        document->set_access_key(document_key);
        document->set_full_access(true);
        return document;
    }

    string document_key;
};

}  // namespace

TEST(ProtectedAtomDBTest, RejectsNullBackend) { EXPECT_THROW(ProtectedAtomDB(nullptr), runtime_error); }

TEST(ProtectedAtomDBTest, ProtectedMethodsRequireKeychain) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(false);
    Node node("Symbol", "\"node\"");
    LinkSchema schema("Expression", 2);

    EXPECT_THROW(protected_atomdb->db->get_atom("handle"), runtime_error);
    EXPECT_THROW(protected_atomdb->db->query_for_pattern(schema), runtime_error);
    EXPECT_THROW(protected_atomdb->db->atom_exists("handle"), runtime_error);
    EXPECT_THROW(protected_atomdb->db->atoms_exist({"handle"}), runtime_error);
    EXPECT_THROW(protected_atomdb->db->add_node(&node), runtime_error);
    EXPECT_THROW(protected_atomdb->db->atom_count(), runtime_error);
}

TEST(ProtectedAtomDBTest, KeychainWithoutDatabaseKeyDeniesAccess) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    protected_atomdb->grant_full_access(PKAdmin);
    Animals animals;
    LinkSchema similarity_human_schema(similarity_human_tokens());
    auto granted_link = protected_atomdb->backend->get_link(animals.similarity_human_monkey);
    ASSERT_NE(granted_link, nullptr);

    vector<shared_ptr<Keychain>> missing_keys = {
        nullptr,
        make_keychain(protected_atomdb->db->get_uid(), ""),
        make_keychain("other_uid", PKAdmin),
    };
    for (const auto& keys : missing_keys) {
        EXPECT_EQ(protected_atomdb->db->get_atom(animals.similarity_human_monkey, keys), nullptr);
    }

    shared_ptr<Keychain> keys = nullptr;
    EXPECT_TRUE(protected_atomdb->db->get_matching_atoms(false, *granted_link, keys).empty());
    EXPECT_EQ(protected_atomdb->db->query_for_pattern(similarity_human_schema, keys)->size(), 0u);
    EXPECT_EQ(protected_atomdb->db->query_for_targets(animals.similarity_human_monkey, keys)->size(),
              0u);
    EXPECT_EQ(protected_atomdb->db->query_for_incoming_set(animals.human, keys)->size(), 0u);
    EXPECT_FALSE(protected_atomdb->db->atom_exists(animals.similarity_human_monkey, keys));
    EXPECT_TRUE(protected_atomdb->db->atoms_exist({animals.similarity_human_monkey}, keys).empty());
}

TEST(ProtectedAtomDBTest, UnknownPublicKeyDeniesAccess) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    auto unknown_keys = protected_atomdb->keys(PKUnknown);
    Animals animals;

    EXPECT_EQ(protected_atomdb->db->get_atom(animals.similarity_human_monkey, unknown_keys), nullptr);
    EXPECT_FALSE(protected_atomdb->db->link_exists(animals.similarity_human_monkey, unknown_keys));
    EXPECT_TRUE(
        protected_atomdb->db->links_exist({animals.similarity_human_monkey}, unknown_keys).empty());
    EXPECT_EQ(protected_atomdb->db->query_for_incoming_set(animals.human, unknown_keys)->size(), 0u);
}

TEST(ProtectedAtomDBTest, CachedAuthorizationSurvivesBackendRevoke) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    protected_atomdb->grant_full_access(PKAdmin);
    auto admin_keys = protected_atomdb->keys(PKAdmin);
    Animals animals;

    ASSERT_NE(protected_atomdb->db->get_atom(animals.similarity_human_monkey, admin_keys), nullptr);
    protected_atomdb->persistence->revoke(PKAdmin);
    EXPECT_NE(protected_atomdb->db->get_atom(animals.similarity_human_monkey, admin_keys), nullptr);
}

TEST(ProtectedAtomDBTest, LoadedPermissionDocumentMustMatchRequestedKey) {
    Node node("Symbol", "\"human\"");

    auto matching_backend = make_shared<AccessDocumentBackend>("pk");
    string handle = matching_backend->add_node(&node);
    ProtectedAtomDB matching_db(matching_backend);
    auto keys = make_keychain(matching_db.get_uid(), "pk");
    EXPECT_NE(matching_db.get_atom(handle, keys), nullptr);

    auto mismatched_backend = make_shared<AccessDocumentBackend>("other_key");
    mismatched_backend->add_node(&node);
    ProtectedAtomDB mismatched_db(mismatched_backend);
    EXPECT_EQ(mismatched_db.get_atom(handle, keys), nullptr);
}

TEST(ProtectedAtomDBTest, ReadOperationsReturnOnlyAuthorizedAtoms) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    protected_atomdb->grant_full_access(PKAdmin);
    protected_atomdb->grant_link_template(PKSimilarityHuman, similarity_human_tokens());
    auto admin_keys = protected_atomdb->keys(PKAdmin);
    auto similarity_human_keys = protected_atomdb->keys(PKSimilarityHuman);
    Animals animals;

    EXPECT_NE(protected_atomdb->db->get_link(animals.similarity_human_monkey, admin_keys), nullptr);
    EXPECT_NE(protected_atomdb->db->get_link(animals.similarity_snake_vine, admin_keys), nullptr);
    EXPECT_NE(protected_atomdb->db->get_node(animals.human, admin_keys), nullptr);
    EXPECT_TRUE(protected_atomdb->db->atom_exists(animals.similarity_human_monkey, admin_keys));
    EXPECT_TRUE(protected_atomdb->db->node_exists(animals.human, admin_keys));
    EXPECT_TRUE(protected_atomdb->db->link_exists(animals.inheritance_human_mammal, admin_keys));

    EXPECT_NE(protected_atomdb->db->get_link(animals.similarity_human_monkey, similarity_human_keys),
              nullptr);
    EXPECT_NE(protected_atomdb->db->get_link(animals.similarity_human_chimp, similarity_human_keys),
              nullptr);
    EXPECT_NE(protected_atomdb->db->get_link(animals.similarity_human_ent, similarity_human_keys),
              nullptr);
    EXPECT_EQ(protected_atomdb->db->get_link(animals.similarity_snake_vine, similarity_human_keys),
              nullptr);
    EXPECT_EQ(protected_atomdb->db->get_link(animals.inheritance_human_mammal, similarity_human_keys),
              nullptr);
    EXPECT_EQ(protected_atomdb->db->get_node(animals.human, similarity_human_keys), nullptr);

    EXPECT_TRUE(
        protected_atomdb->db->link_exists(animals.similarity_human_monkey, similarity_human_keys));
    EXPECT_FALSE(
        protected_atomdb->db->link_exists(animals.similarity_snake_vine, similarity_human_keys));
    EXPECT_FALSE(
        protected_atomdb->db->atom_exists(animals.inheritance_human_mammal, similarity_human_keys));
    EXPECT_FALSE(protected_atomdb->db->node_exists(animals.human, similarity_human_keys));

    EXPECT_EQ(protected_atomdb->db->get_atom("missing", admin_keys), nullptr);
    EXPECT_FALSE(protected_atomdb->db->atom_exists("missing", admin_keys));
    EXPECT_EQ(protected_atomdb->db->get_atom(animals.similarity_snake_vine, similarity_human_keys),
              nullptr);
    EXPECT_FALSE(
        protected_atomdb->db->atom_exists(animals.similarity_snake_vine, similarity_human_keys));

    EXPECT_EQ(protected_atomdb->db->links_exist({animals.similarity_human_monkey,
                                                 animals.similarity_human_chimp,
                                                 animals.similarity_human_ent,
                                                 animals.similarity_snake_vine,
                                                 animals.inheritance_human_mammal},
                                                similarity_human_keys),
              set<string>({animals.similarity_human_monkey,
                           animals.similarity_human_chimp,
                           animals.similarity_human_ent}));
    EXPECT_EQ(protected_atomdb->db->atoms_exist({animals.similarity_human_monkey,
                                                 animals.similarity_human_chimp,
                                                 animals.similarity_human_ent,
                                                 animals.human,
                                                 animals.similarity_snake_vine},
                                                similarity_human_keys),
              set<string>({animals.similarity_human_monkey,
                           animals.similarity_human_chimp,
                           animals.similarity_human_ent}));
    EXPECT_TRUE(protected_atomdb->db->nodes_exist({animals.human, animals.monkey}, similarity_human_keys)
                    .empty());
    EXPECT_EQ(protected_atomdb->db->nodes_exist({animals.human}, admin_keys),
              set<string>({animals.human}));
}

TEST(ProtectedAtomDBTest, ExistenceChecksPreserveNodeAndLinkTypes) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    protected_atomdb->grant_full_access(PKAdmin);
    auto admin_keys = protected_atomdb->keys(PKAdmin);
    Animals animals;

    ASSERT_TRUE(protected_atomdb->db->atom_exists(animals.human, admin_keys));
    ASSERT_TRUE(protected_atomdb->db->atom_exists(animals.similarity_human_monkey, admin_keys));

    EXPECT_TRUE(protected_atomdb->db->node_exists(animals.human, admin_keys));
    EXPECT_FALSE(protected_atomdb->db->node_exists(animals.similarity_human_monkey, admin_keys));
    EXPECT_TRUE(protected_atomdb->db->link_exists(animals.similarity_human_monkey, admin_keys));
    EXPECT_FALSE(protected_atomdb->db->link_exists(animals.human, admin_keys));

    EXPECT_EQ(
        protected_atomdb->db->nodes_exist({animals.human, animals.similarity_human_monkey}, admin_keys),
        set<string>({animals.human}));
    EXPECT_EQ(
        protected_atomdb->db->links_exist({animals.human, animals.similarity_human_monkey}, admin_keys),
        set<string>({animals.similarity_human_monkey}));
    EXPECT_EQ(
        protected_atomdb->db->atoms_exist({animals.human, animals.similarity_human_monkey}, admin_keys),
        set<string>({animals.human, animals.similarity_human_monkey}));
}

TEST(ProtectedAtomDBTest, QueryForPatternReturnsOnlyAuthorizedHandles) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    protected_atomdb->grant_full_access(PKAdmin);
    protected_atomdb->grant_link_template(PKSimilarityHuman, similarity_human_tokens());
    auto admin_keys = protected_atomdb->keys(PKAdmin);
    auto similarity_human_keys = protected_atomdb->keys(PKSimilarityHuman);
    Animals animals;

    LinkSchema similarity_human_schema(similarity_human_tokens());
    auto answers1 = handles_from_set(
        protected_atomdb->db->query_for_pattern(similarity_human_schema, similarity_human_keys));
    EXPECT_EQ(answers1.size(), 3);
    EXPECT_TRUE(answers1.count(animals.similarity_human_monkey));
    EXPECT_TRUE(answers1.count(animals.similarity_human_chimp));
    EXPECT_TRUE(answers1.count(animals.similarity_human_ent));
    EXPECT_FALSE(answers1.count(animals.similarity_snake_vine));

    LinkSchema similarity_snake_schema({"LINK_TEMPLATE",
                                        "Expression",
                                        "3",
                                        "NODE",
                                        "Symbol",
                                        "Similarity",
                                        "NODE",
                                        "Symbol",
                                        "\"snake\"",
                                        "VARIABLE",
                                        "V"});
    auto answers2 = handles_from_set(
        protected_atomdb->db->query_for_pattern(similarity_snake_schema, similarity_human_keys));
    EXPECT_TRUE(answers2.empty());

    auto answers3 =
        handles_from_set(protected_atomdb->db->query_for_pattern(similarity_snake_schema, admin_keys));
    EXPECT_EQ(answers3.size(), 2);
    EXPECT_TRUE(answers3.count(animals.similarity_snake_earthworm));
    EXPECT_TRUE(answers3.count(animals.similarity_snake_vine));

    auto granted = protected_atomdb->backend->get_link(animals.similarity_human_monkey);
    auto denied = protected_atomdb->backend->get_link(animals.similarity_snake_vine);
    ASSERT_NE(granted, nullptr);
    ASSERT_NE(denied, nullptr);
    EXPECT_EQ(protected_atomdb->db->get_matching_atoms(false, *granted, similarity_human_keys).size(),
              1);
    EXPECT_TRUE(protected_atomdb->db->get_matching_atoms(false, *denied, similarity_human_keys).empty());
}

TEST(ProtectedAtomDBTest, IncomingSetRequiresReadableHandleAndFiltersResults) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    protected_atomdb->grant_full_access(PKAdmin);
    protected_atomdb->grant_link_template(PKSimilarityHuman, similarity_human_tokens());
    auto admin_keys = protected_atomdb->keys(PKAdmin);
    auto similarity_human_keys = protected_atomdb->keys(PKSimilarityHuman);
    Animals animals;

    auto admin_incoming_human =
        handles_from_set(protected_atomdb->db->query_for_incoming_set(animals.human, admin_keys));
    EXPECT_EQ(admin_incoming_human,
              set<string>({animals.similarity_human_monkey,
                           animals.similarity_human_chimp,
                           animals.similarity_human_ent,
                           animals.similarity_monkey_human,
                           animals.similarity_chimp_human,
                           animals.similarity_ent_human,
                           animals.inheritance_human_mammal}));

    EXPECT_TRUE(handles_from_set(
                    protected_atomdb->db->query_for_incoming_set(animals.human, similarity_human_keys))
                    .empty());

    auto admin_incoming_similarity_human_monkey = handles_from_set(
        protected_atomdb->db->query_for_incoming_set(animals.similarity_human_monkey, admin_keys));
    EXPECT_EQ(admin_incoming_similarity_human_monkey,
              set<string>({animals.related_similarity_human_monkey_similarity_human_chimp,
                           animals.related_similarity_human_monkey_similarity_chimp_monkey,
                           animals.related_similarity_human_monkey_similarity_human_ent,
                           animals.related_similarity_human_monkey_similarity_monkey_human,
                           animals.related_similarity_human_monkey_similarity_chimp_human,
                           animals.related_similarity_human_monkey_similarity_monkey_chimp,
                           animals.related_similarity_human_monkey_similarity_ent_human,
                           animals.related_similarity_and_inheritance,
                           animals.related_similarity_human_monkey_inheritance_monkey_mammal}));
    auto incoming_similarity_human_monkey =
        handles_from_set(protected_atomdb->db->query_for_incoming_set(animals.similarity_human_monkey,
                                                                      similarity_human_keys));
    EXPECT_TRUE(incoming_similarity_human_monkey.empty());
}

TEST(ProtectedAtomDBTest, TargetsReturnOutgoingWithoutFiltering) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    protected_atomdb->grant_full_access(PKAdmin);
    protected_atomdb->grant_link_template(PKSimilarityHuman, similarity_human_tokens());
    auto admin_keys = protected_atomdb->keys(PKAdmin);
    auto similarity_human_keys = protected_atomdb->keys(PKSimilarityHuman);
    Animals animals;

    auto admin_targets = handles_from_list(
        protected_atomdb->db->query_for_targets(animals.similarity_human_monkey, admin_keys));
    ASSERT_EQ(admin_targets.size(), 3);
    EXPECT_EQ(admin_targets[0], animals.similarity);
    EXPECT_EQ(admin_targets[1], animals.human);
    EXPECT_EQ(admin_targets[2], animals.monkey);

    EXPECT_EQ(handles_from_list(protected_atomdb->db->query_for_targets(animals.similarity_human_monkey,
                                                                        similarity_human_keys)),
              admin_targets);
    EXPECT_EQ(protected_atomdb->db->get_node(animals.human, similarity_human_keys), nullptr);

    EXPECT_TRUE(handles_from_list(protected_atomdb->db->query_for_targets(animals.similarity_snake_vine,
                                                                          similarity_human_keys))
                    .empty());

    auto missing_targets = protected_atomdb->db->query_for_targets("missing", admin_keys);
    ASSERT_NE(missing_targets, nullptr);
    EXPECT_EQ(missing_targets->size(), 0u);
    EXPECT_EQ(protected_atomdb->backend->query_for_targets("missing"), nullptr);
}

TEST(ProtectedAtomDBTest, ParentAndChildGrantsAreIndependent) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    Animals animals;
    string A = animals.similarity_human_monkey;
    string B = animals.similarity_snake_vine;
    string C = animals.inheritance_human_mammal;
    Link link("Expression", {A, B, C});
    string H = protected_atomdb->backend->add_link(&link);

    protected_atomdb->grant_link_template(
        PKOnlyH, {"LINK_TEMPLATE", "Expression", "3", "ATOM", A, "ATOM", B, "VARIABLE", "v"});
    protected_atomdb->persistence->revoke(PKOnlyABC);
    vector<pair<LinkSchema, unsigned int>> only_abc_schemas = {
        {LinkSchema(similarity_human_tokens()), 1},
        {LinkSchema({"LINK_TEMPLATE",
                     "Expression",
                     "3",
                     "NODE",
                     "Symbol",
                     "Similarity",
                     "VARIABLE",
                     "V",
                     "NODE",
                     "Symbol",
                     "\"vine\""}),
         1},
        {LinkSchema({"LINK_TEMPLATE",
                     "Expression",
                     "3",
                     "NODE",
                     "Symbol",
                     "Inheritance",
                     "NODE",
                     "Symbol",
                     "\"human\"",
                     "VARIABLE",
                     "V"}),
         1}};
    protected_atomdb->persistence->grant(PKOnlyABC, only_abc_schemas);
    auto only_h_keys = protected_atomdb->keys(PKOnlyH);
    auto only_abc_keys = protected_atomdb->keys(PKOnlyABC);

    EXPECT_NE(protected_atomdb->db->get_atom(H, only_h_keys), nullptr);
    EXPECT_EQ(protected_atomdb->db->get_atom(A, only_h_keys), nullptr);
    EXPECT_EQ(protected_atomdb->db->get_atom(B, only_h_keys), nullptr);
    EXPECT_EQ(protected_atomdb->db->get_atom(C, only_h_keys), nullptr);

    EXPECT_EQ(protected_atomdb->db->get_atom(H, only_abc_keys), nullptr);
    EXPECT_NE(protected_atomdb->db->get_atom(A, only_abc_keys), nullptr);
    EXPECT_NE(protected_atomdb->db->get_atom(B, only_abc_keys), nullptr);
    EXPECT_NE(protected_atomdb->db->get_atom(C, only_abc_keys), nullptr);

    auto targets = handles_from_list(protected_atomdb->db->query_for_targets(H, only_h_keys));
    ASSERT_EQ(targets.size(), 3);
    EXPECT_EQ(targets[0], A);
    EXPECT_EQ(targets[1], B);
    EXPECT_EQ(targets[2], C);

    EXPECT_TRUE(handles_from_list(protected_atomdb->db->query_for_targets(H, only_abc_keys)).empty());

    EXPECT_TRUE(handles_from_set(protected_atomdb->db->query_for_incoming_set(A, only_h_keys)).empty());
    EXPECT_FALSE(
        handles_from_set(protected_atomdb->db->query_for_incoming_set(A, only_abc_keys)).count(H));
}

TEST(ProtectedAtomDBTest, NestedRelatedGrantDoesNotImplyInnerSimilarity) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    vector<string> related_of_similarity_human = {"LINK_TEMPLATE",
                                                  "Expression",
                                                  "3",
                                                  "NODE",
                                                  "Symbol",
                                                  "Related",
                                                  "LINK_TEMPLATE",
                                                  "Expression",
                                                  "3",
                                                  "NODE",
                                                  "Symbol",
                                                  "Similarity",
                                                  "NODE",
                                                  "Symbol",
                                                  "\"human\"",
                                                  "VARIABLE",
                                                  "V1",
                                                  "VARIABLE",
                                                  "V2"};
    protected_atomdb->grant_link_template(PKRelatedHuman, related_of_similarity_human);
    protected_atomdb->grant_link_template(PKSimilarityHuman, similarity_human_tokens());
    auto related_human_keys = protected_atomdb->keys(PKRelatedHuman);
    auto similarity_human_keys = protected_atomdb->keys(PKSimilarityHuman);
    Animals animals;

    EXPECT_NE(protected_atomdb->backend->get_link(animals.related_similarity_and_inheritance), nullptr);
    EXPECT_NE(
        protected_atomdb->db->get_link(animals.related_similarity_and_inheritance, related_human_keys),
        nullptr);
    EXPECT_EQ(protected_atomdb->db->get_link(animals.related_similarity_and_inheritance,
                                             similarity_human_keys),
              nullptr);
    EXPECT_EQ(protected_atomdb->db->get_link(animals.similarity_human_monkey, related_human_keys),
              nullptr);

    LinkSchema similarity_human_schema(similarity_human_tokens());
    EXPECT_TRUE(handles_from_set(
                    protected_atomdb->db->query_for_pattern(similarity_human_schema, related_human_keys))
                    .empty());

    LinkSchema related_similarity_human_schema(related_of_similarity_human);
    EXPECT_TRUE(handles_from_set(protected_atomdb->db->query_for_pattern(related_similarity_human_schema,
                                                                         related_human_keys))
                    .count(animals.related_similarity_and_inheritance));
    EXPECT_TRUE(handles_from_set(protected_atomdb->db->query_for_pattern(related_similarity_human_schema,
                                                                         similarity_human_keys))
                    .empty());
}
