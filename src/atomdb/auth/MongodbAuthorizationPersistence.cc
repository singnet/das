#include "MongodbAuthorizationPersistence.h"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/options/replace.hpp>
#include <utility>

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
    if (endpoint.empty() || endpoint == ":" || username.empty() || password.empty() ||
        database_name.empty() || collection_name.empty()) {
        RAISE_ERROR(
            "Invalid MongoDB configuration: need non-empty endpoint, username, password, database_name "
            "and collection_name.");
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

void MongodbAuthorizationPersistence::authorize(const string& public_key,
                                                vector<pair<LinkSchema, unsigned int>>& schemas) {
    auto conn = this->mongodb_pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];

    auto document = this->get_document(collection, public_key);
    auto bson = document ? this->to_bson(*document, schemas) : this->to_bson(public_key, schemas);

    auto filter = bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp("_id", Hasher::plain_string_hash(public_key)));

    mongocxx::options::replace opts;
    opts.upsert(true);

    auto reply = collection.replace_one(filter.view(), bson.view(), opts);

    if (!reply) {
        RAISE_ERROR("Failed to update authorization entry in MongoDB");
    }
}

void MongodbAuthorizationPersistence::revoke(const string& public_key) {
    auto conn = this->mongodb_pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];
    auto reply = collection.delete_one(bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp("_id", Hasher::plain_string_hash(public_key))));
    if (!reply) {
        RAISE_ERROR("Failed to remove authorization document from MongoDB");
    }
}

// --------------------------------------------------------------------------------
// Private methods

bsoncxx::document::value MongodbAuthorizationPersistence::to_bson(
    const string& public_key, vector<pair<LinkSchema, unsigned int>>& schemas) {
    bsoncxx::builder::basic::array allowed_schemas;
    append_schema_entries(allowed_schemas, schemas);

    return bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp("_id", Hasher::plain_string_hash(public_key)),
        bsoncxx::builder::basic::kvp("public_key", public_key),
        bsoncxx::builder::basic::kvp("full_access", false),
        bsoncxx::builder::basic::kvp("allowed_schemas", allowed_schemas));
}

bsoncxx::document::value MongodbAuthorizationPersistence::to_bson(
    const atomdb_api_types::MongodbAccessPermissionDocument& document,
    vector<pair<LinkSchema, unsigned int>>& schemas) {
    bsoncxx::builder::basic::array allowed_schemas;

    for (unsigned int i = 0; i < document.get_entries_size(); ++i) {
        const auto& entry = document.get_entry(i);
        auto tokens_array = bsoncxx::builder::basic::array{};
        for (unsigned int j = 0; j < entry.get_tokens_size(); ++j) {
            tokens_array.append(entry.get_token(j));
        }
        allowed_schemas.append(bsoncxx::builder::basic::make_document(
            bsoncxx::builder::basic::kvp("tokens", tokens_array),
            bsoncxx::builder::basic::kvp("read", entry.get_read()),
            bsoncxx::builder::basic::kvp("write", entry.get_write())));
    }

    append_schema_entries(allowed_schemas, schemas);

    return bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp("_id", Hasher::plain_string_hash(document.get_access_key())),
        bsoncxx::builder::basic::kvp("public_key", document.get_access_key()),
        bsoncxx::builder::basic::kvp("full_access", document.get_full_access()),
        bsoncxx::builder::basic::kvp("allowed_schemas", allowed_schemas));
}

shared_ptr<atomdb_api_types::MongodbAccessPermissionDocument>
MongodbAuthorizationPersistence::get_document(mongocxx::collection& collection,
                                              const string& public_key) {
    auto reply = collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp("_id", Hasher::plain_string_hash(public_key))));

    if (!reply) return nullptr;

    return make_shared<atomdb_api_types::MongodbAccessPermissionDocument>(reply.value());
}

void MongodbAuthorizationPersistence::append_schema_entries(
    bsoncxx::builder::basic::array& allowed_schemas, vector<pair<LinkSchema, unsigned int>>& schemas) {
    for (auto& [schema, permission] : schemas) {
        auto tokens_array = bsoncxx::builder::basic::array{};
        for (const auto& token : schema.tokens()) {
            tokens_array.append(token);
        }

        bool read, write;

        if (permission == 1) {
            read = true;
            write = false;
        } else if (permission == 2) {
            read = false;
            write = true;
        } else if (permission == 3) {
            read = true;
            write = true;
        } else {
            RAISE_ERROR("Invalid permission value: " + to_string(permission));
        }

        allowed_schemas.append(
            bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("tokens", tokens_array),
                                                   bsoncxx::builder::basic::kvp("read", read),
                                                   bsoncxx::builder::basic::kvp("write", write)));
    }
}
