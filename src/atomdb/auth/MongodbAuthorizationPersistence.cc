#include "MongodbAuthorizationPersistence.h"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/options/replace.hpp>

#include "Hasher.h"
#include "MongoInitializer.h"
#include "Utils.h"
#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace commons;

// --------------------------------------------------------------------------------
// Constructors

MongodbAuthorizationPersistence::MongodbAuthorizationPersistence(const string& endpoint,
                                                                 const string& username,
                                                                 const string& password,
                                                                 const string& database_name,
                                                                 const string& collection_name)
    : database_name(database_name), collection_name(collection_name) {
    if (endpoint.empty() || endpoint == ":" || username.empty() || password.empty() || database_name.empty() || collection_name.empty()) {
        RAISE_ERROR("Invalid MongoDB configuration: need non-empty endpoint, username, password, database_name and collection_name.");
    }

    string url = "mongodb://" + username + ":" + password + "@" + endpoint;

    MongoInitializer::initialize();

    try {
        auto uri = mongocxx::uri{url};
        this->mongodb_pool = new mongocxx::pool(uri);
        auto conn = this->mongodb_pool->acquire();
        auto mongodb = (*conn)[database_name];
        const auto ping_cmd =
            bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1));
        mongodb.run_command(ping_cmd.view());
        LOG_DEBUG("MongodbAuthorizationPersistence connected to MongoDB at "
                  << endpoint << " (db=" << database_name << ")");
    } catch (const exception& e) {
        RAISE_ERROR(e.what());
    }
}

MongodbAuthorizationPersistence::~MongodbAuthorizationPersistence() { delete this->mongodb_pool; }

// --------------------------------------------------------------------------------
// Public methods

vector<AuthorizationSchema> MongodbAuthorizationPersistence::list(const string& public_key) {
    auto conn = this->mongodb_pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];
    auto document = this->get_document(collection, public_key);
    if (!document) return {};
    auto profile = AuthorizationProfile::from_document(*document);
    return profile.schemas();
}

void MongodbAuthorizationPersistence::save(const string& public_key, const AuthorizationSchema& schema) {
    auto conn = this->mongodb_pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];

    auto document = this->get_document(collection, public_key);
    auto profile = document ? AuthorizationProfile::from_document(*document).with_schema(schema)
                            : AuthorizationProfile(public_key, false, {schema});

    auto filter = bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp("_id", Hasher::plain_string_hash(public_key)));

    mongocxx::options::replace opts;
    opts.upsert(true);

    auto bson = to_bson(profile);
    auto reply = collection.replace_one(filter.view(), bson.view(), opts);

    if (!reply) {
        RAISE_ERROR("Failed to update authorization entry in MongoDB");
    }
}

void MongodbAuthorizationPersistence::remove(const string& public_key,
                                             const AuthorizationSchema& schema) {
    auto conn = this->mongodb_pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];

    auto document = this->get_document(collection, public_key);
    if (!document) return;

    auto profile = AuthorizationProfile::from_document(*document).without_schema(schema);
    if (!profile) {
        this->delete_document(collection, public_key);
        return;
    }

    auto filter = bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp("_id", Hasher::plain_string_hash(public_key)));

    auto bson = to_bson(*profile);
    auto reply = collection.replace_one(filter.view(), bson.view());

    if (!reply) {
        RAISE_ERROR("Failed to update authorization entry in MongoDB");
    }
}

void MongodbAuthorizationPersistence::remove_all(const string& public_key) {
    auto conn = this->mongodb_pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];
    this->delete_document(collection, public_key);
}

// --------------------------------------------------------------------------------
// Private methods

bsoncxx::document::value MongodbAuthorizationPersistence::to_bson(const AuthorizationProfile& profile) {
    bsoncxx::builder::basic::array schemas;
    for (const auto& schema : profile.schemas()) {
        auto tokens_array = bsoncxx::builder::basic::array{};
        for (const auto& token : LinkSchema(schema.schema()).tokenize()) {
            tokens_array.append(token);
        }
        schemas.append(bsoncxx::builder::basic::make_document(
            bsoncxx::builder::basic::kvp("tokens", tokens_array),
            bsoncxx::builder::basic::kvp("read", schema.read()),
            bsoncxx::builder::basic::kvp("write", schema.write())));
    }

    return bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp("_id", Hasher::plain_string_hash(profile.access_key())),
        bsoncxx::builder::basic::kvp("public_key", profile.access_key()),
        bsoncxx::builder::basic::kvp("full_access", profile.is_full_access()),
        bsoncxx::builder::basic::kvp("allowed_schemas", schemas));
}

shared_ptr<atomdb_api_types::MongodbAccessPermissionDocument>
MongodbAuthorizationPersistence::get_document(mongocxx::collection& collection,
                                              const string& public_key) {
    auto reply = collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp("_id", Hasher::plain_string_hash(public_key))));

    if (!reply) return nullptr;

    return make_shared<atomdb_api_types::MongodbAccessPermissionDocument>(reply.value());
}

void MongodbAuthorizationPersistence::delete_document(mongocxx::collection& collection,
                                                      const string& public_key) {
    auto reply = collection.delete_one(bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp("_id", Hasher::plain_string_hash(public_key))));
    if (!reply) {
        RAISE_ERROR("Failed to remove authorization document from MongoDB");
    }
}
