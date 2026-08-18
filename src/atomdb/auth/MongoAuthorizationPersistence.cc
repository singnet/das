#include "MongoAuthorizationPersistence.h"

#include "Hasher.h"
#include "Utils.h"
#include "expression_hasher.h"

using namespace atomdb;
using namespace commons;

// --------------------------------------------------------------------------------
// Constructors

MongoAuthorizationPersistence::MongoAuthorizationPersistence(mongocxx::pool* pool,
                                                             const string& database_name,
                                                             const string& collection_name)
    : pool(pool), database_name(database_name), collection_name(collection_name) {
    if (this->pool == nullptr) {
        RAISE_ERROR("MongoAuthorizationPersistence requires a non-null MongoDB pool");
    }
    if (this->database_name.empty() || this->collection_name.empty()) {
        RAISE_ERROR("MongoAuthorizationPersistence requires a non-empty database and collection names");
    }
}

// --------------------------------------------------------------------------------
// Public methods

void MongoAuthorizationPersistence::save(const atomdb_api_types::PublicKey& public_key,
                                         const atomdb_api_types::AccessPermissionEntry& entry) {
    auto conn = this->pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];

    for (const auto& key : public_key.keys) {
        auto access_document = this->get_document(collection, key);

        string id;
        string public_key_;
        bool full_access = false;
        auto schemas = bsoncxx::builder::basic::array{};

        if (access_document) {
            id = Hasher::plain_string_hash(access_document->access_key);
            public_key_ = access_document->access_key;
            full_access = access_document->full_access;

            bool entry_exists = false;

            for (const auto& document_entry : access_document->entries) {
                if (document_entry.schema.handle() == entry.schema.handle()) {
                    schemas.append(this->entry_to_document(entry));
                    entry_exists = true;
                } else {
                    schemas.append(this->entry_to_document(document_entry));
                }
            }

            if (!entry_exists) {
                schemas.append(this->entry_to_document(entry));
            }
        } else {
            id = Hasher::plain_string_hash(key);
            public_key_ = key;
            schemas.append(this->entry_to_document(entry));
        }

        auto new_access_document = bsoncxx::builder::basic::make_document(
            bsoncxx::builder::basic::kvp("_id", id),
            bsoncxx::builder::basic::kvp("public_key", public_key_),
            bsoncxx::builder::basic::kvp("full_access", full_access),
            bsoncxx::builder::basic::kvp("allowed_schemas", schemas));

        bsoncxx::builder::stream::document filter;
        filter << "_id" << id;

        mongocxx::options::replace opts;
        opts.upsert(true);

        auto reply = collection.replace_one(filter, new_access_document, opts);

        if (!reply) {
            RAISE_ERROR("Failed to update authorization entry in MongoDB");
        }
    }
}

void MongoAuthorizationPersistence::remove(const atomdb_api_types::PublicKey& public_key,
                                           const atomdb_api_types::AccessPermissionEntry& entry) {
    auto conn = this->pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];

    for (const auto& key : public_key.keys) {
        auto access_document = this->get_document(collection, key);

        if (!access_document) continue;

        auto schemas = bsoncxx::builder::basic::array{};
        for (const auto& document_entry : access_document->entries) {
            if (document_entry.schema.handle() == entry.schema.handle()) {
                continue;  // Skip the entry to remove
            } else {
                schemas.append(this->entry_to_document(document_entry));
            }
        }

        string id = Hasher::plain_string_hash(access_document->access_key);

        auto new_access_document = bsoncxx::builder::basic::make_document(
            bsoncxx::builder::basic::kvp("_id", id),
            bsoncxx::builder::basic::kvp("public_key", access_document->access_key),
            bsoncxx::builder::basic::kvp("full_access", access_document->full_access),
            bsoncxx::builder::basic::kvp("allowed_schemas", schemas));

        bsoncxx::builder::stream::document filter;
        filter << "_id" << id;

        auto reply = collection.replace_one(filter, new_access_document);

        if (!reply) {
            RAISE_ERROR("Failed to update authorization entry in MongoDB");
        }
    }
}

void MongoAuthorizationPersistence::remove_all(const atomdb_api_types::PublicKey& public_key) {
    auto conn = this->pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];
    for (const auto& key : public_key.keys) {
        auto reply = collection.delete_one(bsoncxx::builder::basic::make_document(
            bsoncxx::builder::basic::kvp("_id", Hasher::plain_string_hash(key))));
        if (!reply) {
            RAISE_ERROR("Failed to remove authorization document from MongoDB");
        }
    }
}

// --------------------------------------------------------------------------------
// Private methods

bsoncxx::document::value MongoAuthorizationPersistence::entry_to_document(
    const atomdb_api_types::AccessPermissionEntry& entry) {
    auto tokens_array = bsoncxx::builder::basic::array{};
    for (const auto& token : entry.schema.tokenize()) {
        tokens_array.append(token);
    }
    return bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("tokens", tokens_array),
                                                  bsoncxx::builder::basic::kvp("read", entry.read),
                                                  bsoncxx::builder::basic::kvp("write", entry.write));
}

shared_ptr<atomdb_api_types::AccessPermissionDocument> MongoAuthorizationPersistence::get_document(
    mongocxx::collection& collection, const string& public_key) {
    auto reply = collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp("_id", Hasher::plain_string_hash(public_key))));

    if (!reply) return nullptr;

    auto document_json = nlohmann::json::parse(bsoncxx::to_json(reply->value().view()));

    vector<atomdb_api_types::AccessPermissionEntry> entries;
    for (const auto& item : document_json["allowed_schemas"]) {
        vector<string> tokens;
        for (const auto& token : item["tokens"]) {
            tokens.push_back(token.get<string>());
        }
        entries.emplace_back(tokens, item["read"].get<bool>(), item["write"].get<bool>());
    }

    return make_shared<atomdb_api_types::AccessPermissionDocument>(
        public_key, document_json["full_access"].get<bool>(), entries);
}
