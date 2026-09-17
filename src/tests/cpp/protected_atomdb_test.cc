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

// (Similarity "human" $V)
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

// (Related (Similarity "human" $V1) $V2)
vector<string> related_of_similarity_human_tokens() {
    return {"LINK_TEMPLATE",
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
}

// Expression(A, B, $v). In the animals graph this uniquely matches H = Expression(A, B, C).
// LinkSchema rejects fully-grounded templates, so an exact three-ATOM grant is not possible.
vector<string> expression_ab_variable_tokens(const string& a, const string& b) {
    return {"LINK_TEMPLATE", "Expression", "3", "ATOM", a, "ATOM", b, "VARIABLE", "v"};
}

// (Similarity $V "vine") — uniquely (Similarity "snake" "vine") in this graph.
vector<string> similarity_to_vine_tokens() {
    return {"LINK_TEMPLATE",
            "Expression",
            "3",
            "NODE",
            "Symbol",
            "Similarity",
            "VARIABLE",
            "V",
            "NODE",
            "Symbol",
            "\"vine\""};
}

// (Inheritance "human" $V) — uniquely (Inheritance "human" "mammal") in this graph.
vector<string> inheritance_human_tokens() {
    return {"LINK_TEMPLATE",
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
    string animal = symbol_handle("\"animal\"");
    string reptile = symbol_handle("\"reptile\"");
    string snake = symbol_handle("\"snake\"");
    string dinosaur = symbol_handle("\"dinosaur\"");
    string triceratops = symbol_handle("\"triceratops\"");
    string rhino = symbol_handle("\"rhino\"");
    string earthworm = symbol_handle("\"earthworm\"");
    string vine = symbol_handle("\"vine\"");
    string plant = symbol_handle("\"plant\"");

    string similarity_human_monkey = expression_handle({similarity, human, monkey});
    string similarity_human_chimp = expression_handle({similarity, human, chimp});
    string similarity_chimp_monkey = expression_handle({similarity, chimp, monkey});
    string similarity_snake_earthworm = expression_handle({similarity, snake, earthworm});
    string similarity_rhino_triceratops = expression_handle({similarity, rhino, triceratops});
    string similarity_snake_vine = expression_handle({similarity, snake, vine});
    string similarity_human_ent = expression_handle({similarity, human, ent});
    string similarity_monkey_human = expression_handle({similarity, monkey, human});
    string similarity_chimp_human = expression_handle({similarity, chimp, human});
    string similarity_monkey_chimp = expression_handle({similarity, monkey, chimp});
    string similarity_earthworm_snake = expression_handle({similarity, earthworm, snake});
    string similarity_triceratops_rhino = expression_handle({similarity, triceratops, rhino});
    string similarity_vine_snake = expression_handle({similarity, vine, snake});
    string similarity_ent_human = expression_handle({similarity, ent, human});

    string inheritance_human_mammal = expression_handle({inheritance, human, mammal});
    string inheritance_monkey_mammal = expression_handle({inheritance, monkey, mammal});
    string inheritance_chimp_mammal = expression_handle({inheritance, chimp, mammal});
    string inheritance_mammal_animal = expression_handle({inheritance, mammal, animal});
    string inheritance_reptile_animal = expression_handle({inheritance, reptile, animal});
    string inheritance_snake_reptile = expression_handle({inheritance, snake, reptile});
    string inheritance_dinosaur_reptile = expression_handle({inheritance, dinosaur, reptile});
    string inheritance_triceratops_dinosaur = expression_handle({inheritance, triceratops, dinosaur});
    string inheritance_earthworm_animal = expression_handle({inheritance, earthworm, animal});
    string inheritance_rhino_mammal = expression_handle({inheritance, rhino, mammal});
    string inheritance_vine_plant = expression_handle({inheritance, vine, plant});
    string inheritance_ent_plant = expression_handle({inheritance, ent, plant});

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
    string related_similarity_human_chimp_similarity_chimp_monkey =
        expression_handle({related, similarity_human_chimp, similarity_chimp_monkey});
    string related_similarity_human_chimp_similarity_human_ent =
        expression_handle({related, similarity_human_chimp, similarity_human_ent});
    string related_similarity_human_chimp_similarity_monkey_human =
        expression_handle({related, similarity_human_chimp, similarity_monkey_human});
    string related_similarity_human_chimp_similarity_chimp_human =
        expression_handle({related, similarity_human_chimp, similarity_chimp_human});
    string related_similarity_human_chimp_similarity_monkey_chimp =
        expression_handle({related, similarity_human_chimp, similarity_monkey_chimp});
    string related_similarity_human_chimp_similarity_ent_human =
        expression_handle({related, similarity_human_chimp, similarity_ent_human});
    string related_similarity_human_chimp_inheritance_human_mammal =
        expression_handle({related, similarity_human_chimp, inheritance_human_mammal});
    string related_similarity_human_chimp_inheritance_chimp_mammal =
        expression_handle({related, similarity_human_chimp, inheritance_chimp_mammal});
    string related_similarity_chimp_monkey_similarity_monkey_human =
        expression_handle({related, similarity_chimp_monkey, similarity_monkey_human});
    string related_similarity_chimp_monkey_similarity_chimp_human =
        expression_handle({related, similarity_chimp_monkey, similarity_chimp_human});
    string related_similarity_chimp_monkey_similarity_monkey_chimp =
        expression_handle({related, similarity_chimp_monkey, similarity_monkey_chimp});
    string related_similarity_chimp_monkey_inheritance_monkey_mammal =
        expression_handle({related, similarity_chimp_monkey, inheritance_monkey_mammal});
    string related_similarity_chimp_monkey_inheritance_chimp_mammal =
        expression_handle({related, similarity_chimp_monkey, inheritance_chimp_mammal});
    string related_similarity_snake_earthworm_similarity_snake_vine =
        expression_handle({related, similarity_snake_earthworm, similarity_snake_vine});
    string related_similarity_snake_earthworm_similarity_earthworm_snake =
        expression_handle({related, similarity_snake_earthworm, similarity_earthworm_snake});
    string related_similarity_snake_earthworm_similarity_vine_snake =
        expression_handle({related, similarity_snake_earthworm, similarity_vine_snake});
    string related_similarity_snake_earthworm_inheritance_snake_reptile =
        expression_handle({related, similarity_snake_earthworm, inheritance_snake_reptile});
    string related_similarity_snake_earthworm_inheritance_earthworm_animal =
        expression_handle({related, similarity_snake_earthworm, inheritance_earthworm_animal});
    string related_similarity_rhino_triceratops_similarity_triceratops_rhino =
        expression_handle({related, similarity_rhino_triceratops, similarity_triceratops_rhino});
    string related_similarity_rhino_triceratops_inheritance_triceratops_dinosaur =
        expression_handle({related, similarity_rhino_triceratops, inheritance_triceratops_dinosaur});
    string related_similarity_rhino_triceratops_inheritance_rhino_mammal =
        expression_handle({related, similarity_rhino_triceratops, inheritance_rhino_mammal});
    string related_similarity_snake_vine_similarity_earthworm_snake =
        expression_handle({related, similarity_snake_vine, similarity_earthworm_snake});
    string related_similarity_snake_vine_similarity_vine_snake =
        expression_handle({related, similarity_snake_vine, similarity_vine_snake});
    string related_similarity_snake_vine_inheritance_snake_reptile =
        expression_handle({related, similarity_snake_vine, inheritance_snake_reptile});
    string related_similarity_snake_vine_inheritance_vine_plant =
        expression_handle({related, similarity_snake_vine, inheritance_vine_plant});
    string related_similarity_human_ent_similarity_monkey_human =
        expression_handle({related, similarity_human_ent, similarity_monkey_human});
    string related_similarity_human_ent_similarity_chimp_human =
        expression_handle({related, similarity_human_ent, similarity_chimp_human});
    string related_similarity_human_ent_similarity_ent_human =
        expression_handle({related, similarity_human_ent, similarity_ent_human});
    string related_similarity_human_ent_inheritance_human_mammal =
        expression_handle({related, similarity_human_ent, inheritance_human_mammal});
    string related_similarity_human_ent_inheritance_ent_plant =
        expression_handle({related, similarity_human_ent, inheritance_ent_plant});
    string related_similarity_monkey_human_similarity_chimp_human =
        expression_handle({related, similarity_monkey_human, similarity_chimp_human});
    string related_similarity_monkey_human_similarity_monkey_chimp =
        expression_handle({related, similarity_monkey_human, similarity_monkey_chimp});
    string related_similarity_monkey_human_similarity_ent_human =
        expression_handle({related, similarity_monkey_human, similarity_ent_human});
    string related_similarity_monkey_human_inheritance_human_mammal =
        expression_handle({related, similarity_monkey_human, inheritance_human_mammal});
    string related_similarity_monkey_human_inheritance_monkey_mammal =
        expression_handle({related, similarity_monkey_human, inheritance_monkey_mammal});
    string related_similarity_chimp_human_similarity_monkey_chimp =
        expression_handle({related, similarity_chimp_human, similarity_monkey_chimp});
    string related_similarity_chimp_human_similarity_ent_human =
        expression_handle({related, similarity_chimp_human, similarity_ent_human});
    string related_similarity_chimp_human_inheritance_human_mammal =
        expression_handle({related, similarity_chimp_human, inheritance_human_mammal});
    string related_similarity_chimp_human_inheritance_chimp_mammal =
        expression_handle({related, similarity_chimp_human, inheritance_chimp_mammal});
    string related_similarity_monkey_chimp_inheritance_monkey_mammal =
        expression_handle({related, similarity_monkey_chimp, inheritance_monkey_mammal});
    string related_similarity_monkey_chimp_inheritance_chimp_mammal =
        expression_handle({related, similarity_monkey_chimp, inheritance_chimp_mammal});
    string related_similarity_earthworm_snake_similarity_vine_snake =
        expression_handle({related, similarity_earthworm_snake, similarity_vine_snake});
    string related_similarity_earthworm_snake_inheritance_snake_reptile =
        expression_handle({related, similarity_earthworm_snake, inheritance_snake_reptile});
    string related_similarity_earthworm_snake_inheritance_earthworm_animal =
        expression_handle({related, similarity_earthworm_snake, inheritance_earthworm_animal});
    string related_similarity_triceratops_rhino_inheritance_triceratops_dinosaur =
        expression_handle({related, similarity_triceratops_rhino, inheritance_triceratops_dinosaur});
    string related_similarity_triceratops_rhino_inheritance_rhino_mammal =
        expression_handle({related, similarity_triceratops_rhino, inheritance_rhino_mammal});
    string related_similarity_vine_snake_inheritance_snake_reptile =
        expression_handle({related, similarity_vine_snake, inheritance_snake_reptile});
    string related_similarity_vine_snake_inheritance_vine_plant =
        expression_handle({related, similarity_vine_snake, inheritance_vine_plant});
    string related_similarity_ent_human_inheritance_human_mammal =
        expression_handle({related, similarity_ent_human, inheritance_human_mammal});
    string related_similarity_ent_human_inheritance_ent_plant =
        expression_handle({related, similarity_ent_human, inheritance_ent_plant});
    string related_inheritance_human_mammal_inheritance_monkey_mammal =
        expression_handle({related, inheritance_human_mammal, inheritance_monkey_mammal});
    string related_inheritance_human_mammal_inheritance_chimp_mammal =
        expression_handle({related, inheritance_human_mammal, inheritance_chimp_mammal});
    string related_inheritance_human_mammal_inheritance_mammal_animal =
        expression_handle({related, inheritance_human_mammal, inheritance_mammal_animal});
    string related_inheritance_human_mammal_inheritance_rhino_mammal =
        expression_handle({related, inheritance_human_mammal, inheritance_rhino_mammal});
    string related_inheritance_monkey_mammal_inheritance_chimp_mammal =
        expression_handle({related, inheritance_monkey_mammal, inheritance_chimp_mammal});
    string related_inheritance_monkey_mammal_inheritance_mammal_animal =
        expression_handle({related, inheritance_monkey_mammal, inheritance_mammal_animal});
    string related_inheritance_monkey_mammal_inheritance_rhino_mammal =
        expression_handle({related, inheritance_monkey_mammal, inheritance_rhino_mammal});
    string related_inheritance_chimp_mammal_inheritance_mammal_animal =
        expression_handle({related, inheritance_chimp_mammal, inheritance_mammal_animal});
    string related_inheritance_chimp_mammal_inheritance_rhino_mammal =
        expression_handle({related, inheritance_chimp_mammal, inheritance_rhino_mammal});
    string related_inheritance_mammal_animal_inheritance_reptile_animal =
        expression_handle({related, inheritance_mammal_animal, inheritance_reptile_animal});
    string related_inheritance_mammal_animal_inheritance_earthworm_animal =
        expression_handle({related, inheritance_mammal_animal, inheritance_earthworm_animal});
    string related_inheritance_mammal_animal_inheritance_rhino_mammal =
        expression_handle({related, inheritance_mammal_animal, inheritance_rhino_mammal});
    string related_inheritance_reptile_animal_inheritance_snake_reptile =
        expression_handle({related, inheritance_reptile_animal, inheritance_snake_reptile});
    string related_inheritance_reptile_animal_inheritance_dinosaur_reptile =
        expression_handle({related, inheritance_reptile_animal, inheritance_dinosaur_reptile});
    string related_inheritance_reptile_animal_inheritance_earthworm_animal =
        expression_handle({related, inheritance_reptile_animal, inheritance_earthworm_animal});
    string related_inheritance_snake_reptile_inheritance_dinosaur_reptile =
        expression_handle({related, inheritance_snake_reptile, inheritance_dinosaur_reptile});
    string related_inheritance_dinosaur_reptile_inheritance_triceratops_dinosaur =
        expression_handle({related, inheritance_dinosaur_reptile, inheritance_triceratops_dinosaur});
    string related_inheritance_vine_plant_inheritance_ent_plant =
        expression_handle({related, inheritance_vine_plant, inheritance_ent_plant});
};

JsonConfig protected_redis_mongodb_config() {
    return test_atomdb_json_config("redismongodb", "protected_atomdb_test_", uid);
}

class TestRedisMongoDB : public RedisMongoDB {
   public:
    explicit TestRedisMongoDB(const JsonConfig& config) : RedisMongoDB(config) {}
};

void persist_protected_flag(RedisMongoDB& db, bool is_protected) {
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;
    auto conn = db.get_mongo_pool()->acquire();
    auto collection = (*conn)[db.MONGODB_DB_NAME][db.MONGODB_CONFIG_COLLECTION_NAME];
    collection.delete_many({});
    collection.insert_one(
        make_document(kvp("_id", Hasher::plain_string_hash(db.MONGODB_CONFIG_COLLECTION_NAME)),
                      kvp("protected", is_protected)));
}

struct ProtectedRedisMongo {
    JsonConfig config;
    shared_ptr<TestRedisMongoDB> backend;
    shared_ptr<MongodbAuthorizationPersistence> persistence;
    shared_ptr<ProtectedAtomDB> db;

    explicit ProtectedRedisMongo(bool load_animals) {
        this->config = protected_redis_mongodb_config();

        TestRedisMongoDB seed(this->config);
        seed.drop_all();
        persist_protected_flag(seed, true);
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

    void grant_link_templates(const string& public_key, const vector<vector<string>>& token_lists) {
        this->persistence->revoke(public_key);
        vector<pair<LinkSchema, unsigned int>> schemas;
        for (const auto& tokens : token_lists) {
            schemas.push_back({LinkSchema(tokens), 1});
        }
        this->persistence->grant(public_key, schemas);
    }

    void grant_link_template(const string& public_key, const vector<string>& tokens) {
        this->grant_link_templates(public_key, {tokens});
    }
};

// H = Expression(A, B, C) on top of the animals graph.
// A = (Similarity "human" "monkey")
// B = (Similarity "snake" "vine")
// C = (Inheritance "human" "mammal")
struct LinkH {
    string H;
    string A;
    string B;
    string C;
};

LinkH add_link_h(RedisMongoDB& db) {
    Animals animals;
    LinkH graph;
    graph.A = animals.similarity_human_monkey;
    graph.B = animals.similarity_snake_vine;
    graph.C = animals.inheritance_human_mammal;

    Link link("Expression", {graph.A, graph.B, graph.C});
    graph.H = db.add_link(&link);
    return graph;
}

// Backend that always returns a permission document with a fixed access_key.
// Used to exercise ProtectedAtomDB when the document key does not match the lookup key.
// RedisMongoDB never returns that case: it throws before ProtectedAtomDB sees the document.
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

TEST(ProtectedAtomDBTest, UnprotectedOverloadsRequireKeychain) {
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

TEST(ProtectedAtomDBTest, MissingPublicKeyDeniesAccess) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    protected_atomdb->grant_full_access(PKAdmin);
    Animals animals;
    LinkSchema human_schema(similarity_human_tokens());
    auto granted_link = protected_atomdb->backend->get_link(animals.similarity_human_monkey);
    ASSERT_NE(granted_link, nullptr);

    // Null keychain, empty key, and a keychain for another uid all yield an empty public key.
    vector<shared_ptr<Keychain>> missing_keys = {
        nullptr,
        make_keychain(protected_atomdb->db->get_uid(), ""),
        make_keychain("other_uid", PKAdmin),
    };
    for (const auto& keys : missing_keys) {
        EXPECT_EQ(protected_atomdb->db->get_atom(animals.similarity_human_monkey, keys), nullptr);
    }

    // Same gate for every read: nullptr, false, or an empty collection — never an exception.
    shared_ptr<Keychain> keys = nullptr;
    EXPECT_TRUE(protected_atomdb->db->get_matching_atoms(false, *granted_link, keys).empty());
    EXPECT_EQ(protected_atomdb->db->query_for_pattern(human_schema, keys)->size(), 0u);
    EXPECT_EQ(protected_atomdb->db->query_for_targets(animals.similarity_human_monkey, keys)->size(),
              0u);
    EXPECT_EQ(protected_atomdb->db->query_for_incoming_set(animals.human, keys)->size(), 0u);
    EXPECT_FALSE(protected_atomdb->db->atom_exists(animals.similarity_human_monkey, keys));
    EXPECT_TRUE(protected_atomdb->db->atoms_exist({animals.similarity_human_monkey}, keys).empty());
}

TEST(ProtectedAtomDBTest, UnregisteredPublicKeyDeniesAccess) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    auto unknown_keys = protected_atomdb->keys(PKUnknown);
    Animals animals;

    // No permission document exists for this key. Reads fail the same way as a missing keychain:
    // nullptr, false, or an empty collection — never an exception.
    EXPECT_EQ(protected_atomdb->db->get_atom(animals.similarity_human_monkey, unknown_keys), nullptr);
    EXPECT_FALSE(protected_atomdb->db->link_exists(animals.similarity_human_monkey, unknown_keys));
    EXPECT_TRUE(
        protected_atomdb->db->links_exist({animals.similarity_human_monkey}, unknown_keys).empty());
    EXPECT_EQ(protected_atomdb->db->query_for_incoming_set(animals.human, unknown_keys)->size(), 0u);
}

TEST(ProtectedAtomDBTest, MismatchedAccessKeyDeniesAccess) {
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

TEST(ProtectedAtomDBTest, GrantControlsVisibleAtoms) {
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

    // Denied handles and missing handles are indistinguishable from the caller.
    EXPECT_EQ(protected_atomdb->db->get_atom("missing", admin_keys), nullptr);
    EXPECT_FALSE(protected_atomdb->db->atom_exists("missing", admin_keys));
    EXPECT_EQ(protected_atomdb->db->get_atom(animals.similarity_snake_vine, similarity_human_keys),
              nullptr);
    EXPECT_FALSE(
        protected_atomdb->db->atom_exists(animals.similarity_snake_vine, similarity_human_keys));

    EXPECT_EQ(protected_atomdb->db->links_exist({animals.similarity_human_monkey,
                                                 animals.similarity_snake_vine,
                                                 animals.inheritance_human_mammal},
                                                similarity_human_keys),
              set<string>({animals.similarity_human_monkey}));
    EXPECT_EQ(protected_atomdb->db->atoms_exist(
                  {animals.similarity_human_monkey, animals.human, animals.similarity_snake_vine},
                  similarity_human_keys),
              set<string>({animals.similarity_human_monkey}));
    EXPECT_TRUE(protected_atomdb->db->nodes_exist({animals.human, animals.monkey}, similarity_human_keys)
                    .empty());
    EXPECT_EQ(protected_atomdb->db->nodes_exist({animals.human}, admin_keys),
              set<string>({animals.human}));
}

TEST(ProtectedAtomDBTest, CollectionQueriesDropUnauthorizedHandles) {
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

TEST(ProtectedAtomDBTest, IncomingSetRequiresReadableSeedThenFilters) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    protected_atomdb->grant_full_access(PKAdmin);
    protected_atomdb->grant_link_template(PKSimilarityHuman, similarity_human_tokens());
    auto admin_keys = protected_atomdb->keys(PKAdmin);
    auto similarity_human_keys = protected_atomdb->keys(PKSimilarityHuman);
    Animals animals;

    auto admin_incoming =
        handles_from_set(protected_atomdb->db->query_for_incoming_set(animals.human, admin_keys));
    EXPECT_TRUE(admin_incoming.count(animals.similarity_human_monkey));
    EXPECT_TRUE(admin_incoming.count(animals.inheritance_human_mammal));

    // The Similarity-human grant does not cover the "human" node, so the incoming query is denied.
    EXPECT_TRUE(handles_from_set(
                    protected_atomdb->db->query_for_incoming_set(animals.human, similarity_human_keys))
                    .empty());

    // The seed link is readable, so the query runs. Related links that point to it are not granted
    // and are dropped from the result.
    auto admin_similarity_incoming = handles_from_set(
        protected_atomdb->db->query_for_incoming_set(animals.similarity_human_monkey, admin_keys));
    EXPECT_TRUE(admin_similarity_incoming.count(animals.related_similarity_and_inheritance));
    auto similarity_incoming = handles_from_set(protected_atomdb->db->query_for_incoming_set(
        animals.similarity_human_monkey, similarity_human_keys));
    EXPECT_FALSE(similarity_incoming.count(animals.related_similarity_and_inheritance));
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

    // If the caller can read the link, every outgoing handle is returned, including nodes that
    // the same grant would hide from get_node().
    EXPECT_EQ(handles_from_list(protected_atomdb->db->query_for_targets(animals.similarity_human_monkey,
                                                                        similarity_human_keys)),
              admin_targets);
    EXPECT_EQ(protected_atomdb->db->get_node(animals.human, similarity_human_keys), nullptr);

    EXPECT_TRUE(handles_from_list(protected_atomdb->db->query_for_targets(animals.similarity_snake_vine,
                                                                          similarity_human_keys))
                    .empty());

    // Unauthorized and missing handles both yield an empty list, never nullptr.
    auto missing_targets = protected_atomdb->db->query_for_targets("missing", admin_keys);
    ASSERT_NE(missing_targets, nullptr);
    EXPECT_EQ(missing_targets->size(), 0u);
    EXPECT_EQ(protected_atomdb->backend->query_for_targets("missing"), nullptr);
}

TEST(ProtectedAtomDBTest, ParentAndChildGrantsAreIndependent) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    auto graph = add_link_h(*protected_atomdb->backend);

    protected_atomdb->grant_link_template(PKOnlyH, expression_ab_variable_tokens(graph.A, graph.B));
    protected_atomdb->grant_link_templates(
        PKOnlyABC, {similarity_human_tokens(), similarity_to_vine_tokens(), inheritance_human_tokens()});
    auto only_h = protected_atomdb->keys(PKOnlyH);
    auto only_abc = protected_atomdb->keys(PKOnlyABC);

    EXPECT_NE(protected_atomdb->db->get_atom(graph.H, only_h), nullptr);
    EXPECT_EQ(protected_atomdb->db->get_atom(graph.A, only_h), nullptr);
    EXPECT_EQ(protected_atomdb->db->get_atom(graph.B, only_h), nullptr);
    EXPECT_EQ(protected_atomdb->db->get_atom(graph.C, only_h), nullptr);

    EXPECT_EQ(protected_atomdb->db->get_atom(graph.H, only_abc), nullptr);
    EXPECT_NE(protected_atomdb->db->get_atom(graph.A, only_abc), nullptr);
    EXPECT_NE(protected_atomdb->db->get_atom(graph.B, only_abc), nullptr);
    EXPECT_NE(protected_atomdb->db->get_atom(graph.C, only_abc), nullptr);

    // Readable parent: targets are returned even though A, B and C are not granted.
    auto targets = handles_from_list(protected_atomdb->db->query_for_targets(graph.H, only_h));
    ASSERT_EQ(targets.size(), 3);
    EXPECT_EQ(targets[0], graph.A);
    EXPECT_EQ(targets[1], graph.B);
    EXPECT_EQ(targets[2], graph.C);

    // Unreadable parent: targets are empty even though A, B and C are granted.
    EXPECT_TRUE(handles_from_list(protected_atomdb->db->query_for_targets(graph.H, only_abc)).empty());

    // Incoming requires a readable seed. Granting H does not make incoming(A) visible.
    EXPECT_TRUE(handles_from_set(protected_atomdb->db->query_for_incoming_set(graph.A, only_h)).empty());
    // Seed A is readable, but H is not granted, so it is dropped from incoming(A).
    EXPECT_FALSE(handles_from_set(protected_atomdb->db->query_for_incoming_set(graph.A, only_abc))
                     .count(graph.H));
}

TEST(ProtectedAtomDBTest, NestedRelatedGrantDoesNotImplyInnerSimilarity) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    protected_atomdb->grant_link_template(PKRelatedHuman, related_of_similarity_human_tokens());
    protected_atomdb->grant_link_template(PKSimilarityHuman, similarity_human_tokens());
    auto related_keys = protected_atomdb->keys(PKRelatedHuman);
    auto similarity_keys = protected_atomdb->keys(PKSimilarityHuman);
    Animals animals;

    EXPECT_NE(protected_atomdb->backend->get_link(animals.related_similarity_and_inheritance), nullptr);
    EXPECT_NE(protected_atomdb->db->get_link(animals.related_similarity_and_inheritance, related_keys),
              nullptr);
    EXPECT_EQ(
        protected_atomdb->db->get_link(animals.related_similarity_and_inheritance, similarity_keys),
        nullptr);

    LinkSchema related_schema(related_of_similarity_human_tokens());
    EXPECT_TRUE(handles_from_set(protected_atomdb->db->query_for_pattern(related_schema, related_keys))
                    .count(animals.related_similarity_and_inheritance));
    EXPECT_TRUE(
        handles_from_set(protected_atomdb->db->query_for_pattern(related_schema, similarity_keys))
            .empty());
}

TEST(ProtectedAtomDBTest, KeychainMutationsAreNotImplemented) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(true);
    protected_atomdb->grant_full_access(PKAdmin);
    auto admin_keys = protected_atomdb->keys(PKAdmin);
    Node node("Symbol", "\"node\"");
    Animals animals;

    EXPECT_THROW(protected_atomdb->db->add_node(&node, admin_keys), runtime_error);
    EXPECT_THROW(protected_atomdb->db->delete_link(animals.similarity_human_monkey, admin_keys),
                 runtime_error);
    EXPECT_THROW(protected_atomdb->db->atom_count(admin_keys), runtime_error);
}
