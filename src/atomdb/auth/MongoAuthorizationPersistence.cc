#include "MongoAuthorizationPersistence.h"

#include "Utils.h"

using namespace atomdb;
using namespace commons;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

// --------------------------------------------------------------------------------
// Constructors

MongoAuthorizationPersistence::MongoAuthorizationPersistence(mongocxx::pool* pool,
                                                             const string& database_name,
                                                             const string& collection_name)
    : pool(pool), database_name(database_name), collection_name(collection_name) {
    if (this->pool == nullptr) {
        RAISE_ERROR("MongoAuthorizationPersistence requires a non-null MongoDB pool");
    }
    if (this->database_name.empty()) {
        RAISE_ERROR("MongoAuthorizationPersistence requires a non-empty database name");
    }
    if (this->collection_name.empty()) {
        RAISE_ERROR("MongoAuthorizationPersistence requires a non-empty collection name");
    }
}

// --------------------------------------------------------------------------------
// Public methods

void MongoAuthorizationPersistence::save(const string& public_key, const AuthorizationEntry& entry) {
    auto conn = this->pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];

    auto filter = make_document(kvp("public_key", public_key));
    auto existing = collection.find_one(filter.view());

    bsoncxx::builder::basic::array schemas;
    bool full_access = false;

    if (existing) {
        auto view = existing->view();
        if (view["full_access"] && view["full_access"].type() == bsoncxx::type::k_bool) {
            full_access = view["full_access"].get_bool().value;
        }
        if (view["allowed_schemas"] && view["allowed_schemas"].type() == bsoncxx::type::k_array) {
            string entry_handle = entry.handle();
            for (const auto& item : view["allowed_schemas"].get_array().value) {
                if (item.type() != bsoncxx::type::k_document) {
                    continue;
                }
                auto item_view = item.get_document().view();
                if (item_view["handle"] && item_view["handle"].type() == bsoncxx::type::k_string &&
                    string(item_view["handle"].get_string().value) == entry_handle) {
                    continue;
                }
                schemas.append(item_view);
            }
        }
    }

    schemas.append(make_schema_item(entry));

    auto document = make_document(kvp("_id", public_key),
                                  kvp("public_key", public_key),
                                  kvp("full_access", full_access),
                                  kvp("allowed_schemas", schemas));

    mongocxx::options::replace opts;
    opts.upsert(true);
    auto reply = collection.replace_one(filter.view(), document.view(), opts);
    if (!reply) {
        RAISE_ERROR("Failed to save authorization entry for public_key in MongoDB");
    }
}

void MongoAuthorizationPersistence::remove(const string& public_key, const string& handle) {
    auto conn = this->pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];

    auto filter = make_document(kvp("public_key", public_key));
    auto existing = collection.find_one(filter.view());
    if (!existing) {
        return;
    }

    auto view = existing->view();
    bool full_access = false;
    if (view["full_access"] && view["full_access"].type() == bsoncxx::type::k_bool) {
        full_access = view["full_access"].get_bool().value;
    }

    bsoncxx::builder::basic::array schemas;
    if (view["allowed_schemas"] && view["allowed_schemas"].type() == bsoncxx::type::k_array) {
        for (const auto& item : view["allowed_schemas"].get_array().value) {
            if (item.type() != bsoncxx::type::k_document) {
                continue;
            }
            auto item_view = item.get_document().view();
            if (item_view["handle"] && item_view["handle"].type() == bsoncxx::type::k_string &&
                string(item_view["handle"].get_string().value) == handle) {
                continue;
            }
            // Fallback: recompute handle from tokens when the stored document has no handle field.
            if ((!item_view["handle"] || item_view["handle"].type() != bsoncxx::type::k_string) &&
                item_view["tokens"] && item_view["tokens"].type() == bsoncxx::type::k_array) {
                vector<string> tokens;
                for (const auto& token : item_view["tokens"].get_array().value) {
                    if (token.type() == bsoncxx::type::k_string) {
                        tokens.push_back(string(token.get_string().value));
                    }
                }
                if (!tokens.empty() && AuthorizationEntry(tokens, false, false).handle() == handle) {
                    continue;
                }
            }
            schemas.append(item_view);
        }
    }

    auto document = make_document(kvp("_id", public_key),
                                  kvp("public_key", public_key),
                                  kvp("full_access", full_access),
                                  kvp("allowed_schemas", schemas));

    mongocxx::options::replace opts;
    opts.upsert(false);
    auto reply = collection.replace_one(filter.view(), document.view(), opts);
    if (!reply) {
        RAISE_ERROR("Failed to remove authorization entry for public_key in MongoDB");
    }
}

void MongoAuthorizationPersistence::remove_all(const string& public_key) {
    auto conn = this->pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];
    collection.delete_one(make_document(kvp("public_key", public_key)));
}

// --------------------------------------------------------------------------------
// Private methods

bsoncxx::document::value MongoAuthorizationPersistence::make_schema_item(
    const AuthorizationEntry& entry) {
    auto tokens_array = bsoncxx::builder::basic::array{};
    for (const auto& token : entry.tokenize()) {
        tokens_array.append(token);
    }
    return make_document(kvp("handle", entry.handle()),
                         kvp("tokens", tokens_array),
                         kvp("read", entry.allows(AuthorizationOperation::READ)),
                         kvp("write", entry.allows(AuthorizationOperation::WRITE)));
}