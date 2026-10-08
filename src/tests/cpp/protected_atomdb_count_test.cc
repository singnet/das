#include <gtest/gtest.h>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "AtomDBSingleton.h"
#include "Hasher.h"
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
using namespace atoms;
using namespace commons;
using namespace std;

namespace {

constexpr const char* uid = "animals_db";
constexpr const char* PKAdmin = "pk_admin";
constexpr const char* PKSimilarityHuman = "pk_similarity_human";
constexpr const char* PKUnknown = "pk_unknown";
constexpr const char* PKOnlyH = "pk_only_h";

shared_ptr<Keychain> make_keychain(const string& db_uid, const string& public_key) {
    return make_shared<Keychain>(map<string, string>{{db_uid, public_key}});
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

void expect_counts(const shared_ptr<ProtectedAtomDB>& db,
                   const shared_ptr<Keychain>& keys,
                   size_t nodes,
                   size_t links,
                   size_t atoms) {
    EXPECT_EQ(db->node_count(keys), nodes);
    EXPECT_EQ(db->link_count(keys), links);
    EXPECT_EQ(db->atom_count(keys), atoms);
}

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

        this->config = test_atomdb_json_config("redismongodb", "protected_atomdb_count_test_", uid);

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

}  // namespace

TEST(ProtectedAtomDBTest, CountMethodsWalkGrantedSchemas) {
    shared_ptr<ProtectedRedisMongo> protected_atomdb = make_shared<ProtectedRedisMongo>(false);
    AtomDBSingleton::provide(protected_atomdb->db);

    protected_atomdb->grant_link_template(PKSimilarityHuman, similarity_human_tokens());
    auto similarity_human_keys = protected_atomdb->keys(PKSimilarityHuman);
    expect_counts(protected_atomdb->db, similarity_human_keys, 0, 0, 0);
    expect_counts(protected_atomdb->db, protected_atomdb->keys(PKUnknown), 0, 0, 0);

    protected_atomdb->grant_full_access(PKAdmin);
    expect_counts(protected_atomdb->db, protected_atomdb->keys(PKAdmin), 0, 0, 0);
    expect_counts(protected_atomdb->db, protected_atomdb->keys(PKUnknown), 0, 0, 0);
    expect_counts(protected_atomdb->db, make_keychain(protected_atomdb->db->get_uid(), ""), 0, 0, 0);
    expect_counts(protected_atomdb->db, make_keychain("other_uid", PKAdmin), 0, 0, 0);

    // (Similarity "human" V) matches 3 links. Their node targets fail schema.match, so they are
    // absent from node_count, but each target handle is still added to atom_count: 3 * (1 + 3) = 12.
    Node similarity("Symbol", "Similarity");
    Node human("Symbol", "\"human\"");
    Node monkey("Symbol", "\"monkey\"");
    Node chimp("Symbol", "\"chimp\"");
    Node ent("Symbol", "\"ent\"");
    protected_atomdb->backend->add_node(&similarity);
    protected_atomdb->backend->add_node(&human);
    protected_atomdb->backend->add_node(&monkey);
    protected_atomdb->backend->add_node(&chimp);
    protected_atomdb->backend->add_node(&ent);
    Link similarity_human_monkey("Expression", {similarity.handle(), human.handle(), monkey.handle()});
    Link similarity_human_chimp("Expression", {similarity.handle(), human.handle(), chimp.handle()});
    Link similarity_human_ent("Expression", {similarity.handle(), human.handle(), ent.handle()});
    protected_atomdb->backend->add_link(&similarity_human_monkey);
    protected_atomdb->backend->add_link(&similarity_human_chimp);
    protected_atomdb->backend->add_link(&similarity_human_ent);
    expect_counts(protected_atomdb->db, similarity_human_keys, 0, 3, 12);
    EXPECT_GT(protected_atomdb->db->atom_count(similarity_human_keys),
              protected_atomdb->db->node_count(similarity_human_keys) +
                  protected_atomdb->db->link_count(similarity_human_keys));

    // The loaded profile stays in the manifest, so the count does not drop after revoke.
    protected_atomdb->persistence->revoke(PKSimilarityHuman);
    expect_counts(protected_atomdb->db, similarity_human_keys, 0, 3, 12);

    // (Expression (Expression A B) X). Only the outer link matches the template.
    // The inner link and X fail schema.match: nodes = 0, links = 1, atoms = 3.
    Node node_a("Symbol", "A");
    Node node_b("Symbol", "B");
    Node node_x("Symbol", "X");
    protected_atomdb->backend->add_node(&node_a);
    protected_atomdb->backend->add_node(&node_b);
    protected_atomdb->backend->add_node(&node_x);
    Link inner("Expression", {node_a.handle(), node_b.handle()});
    protected_atomdb->backend->add_link(&inner);
    Link outer("Expression", {inner.handle(), node_x.handle()});
    protected_atomdb->backend->add_link(&outer);
    protected_atomdb->grant_link_template(PKOnlyH,
                                          {"LINK_TEMPLATE",
                                           "Expression",
                                           "2",
                                           "LINK_TEMPLATE",
                                           "Expression",
                                           "2",
                                           "NODE",
                                           "Symbol",
                                           "A",
                                           "NODE",
                                           "Symbol",
                                           "B",
                                           "VARIABLE",
                                           "x"});
    auto nested_keys = protected_atomdb->keys(PKOnlyH);
    expect_counts(protected_atomdb->db, nested_keys, 0, 1, 3);
    EXPECT_GT(
        protected_atomdb->db->atom_count(nested_keys),
        protected_atomdb->db->node_count(nested_keys) + protected_atomdb->db->link_count(nested_keys));
}
