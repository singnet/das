#include "MongoAuthorizationPersistence.h"

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

void MongoAuthorizationPersistence::save(const string& public_key,
                                         const atomdb_api_types::AccessPermissionEntry& entry) {
    string id = this->hashed_id(public_key);
    
    auto document = this->get_document_by_id(id);

    bsoncxx::builder::basic::array schemas;
    
    bool full_access = false;
    
    if (existing) {
        auto view = existing->view();
        full_access = this->read_full_access(view);
        this->append_schemas_except(schemas, view, entry.schema.handle());
    }
    
    schemas.append(this->make_schema_item(entry));

    auto document = bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp("_id", id),
        bsoncxx::builder::basic::kvp("full_access", full_access),
        bsoncxx::builder::basic::kvp("allowed_schemas", schemas)
    );

    mongocxx::options::replace opts;
    
    opts.upsert(true);
    
    auto reply = collection.replace_one(filter.view(), document.view(), opts);
    
    if (!reply) {
        RAISE_ERROR("Failed to save authorization entry in MongoDB");
    }
}

void MongoAuthorizationPersistence::remove(const string& public_key, const string& handle) {
    auto conn = this->pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];

    string id = hashed_id(public_key);
    auto filter = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", id));
    auto existing = collection.find_one(filter.view());
    if (!existing) return;

    auto view = existing->view();
    bsoncxx::builder::basic::array schemas;
    append_schemas_except(schemas, view, handle);

    auto document = bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp("_id", id),
        bsoncxx::builder::basic::kvp("full_access", read_full_access(view)),
        bsoncxx::builder::basic::kvp("allowed_schemas", schemas));

    mongocxx::options::replace opts;
    opts.upsert(false);
    auto reply = collection.replace_one(filter.view(), document.view(), opts);
    if (!reply) {
        RAISE_ERROR("Failed to remove authorization entry in MongoDB");
    }
}

void MongoAuthorizationPersistence::remove_all(const string& public_key) {
    auto conn = this->pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];
    collection.delete_one(bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp("_id", hashed_id(public_key))));
}

// --------------------------------------------------------------------------------
// Private methods

bsoncxx::document::value MongoAuthorizationPersistence::make_schema_item(
    const atomdb_api_types::AccessPermissionEntry& entry) {
    auto tokens_array = bsoncxx::builder::basic::array{};
    for (const auto& token : entry.schema.tokenize()) {
        tokens_array.append(token);
    }
    return bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("tokens", tokens_array),
                                                  bsoncxx::builder::basic::kvp("read", entry.read),
                                                  bsoncxx::builder::basic::kvp("write", entry.write));
}

string MongoAuthorizationPersistence::hashed_id(const string& public_key) {
    return compute_hash((char*) public_key.c_str());
}

string MongoAuthorizationPersistence::schema_handle_from_item(bsoncxx::document::view item) {
    if (!item["tokens"] || item["tokens"].type() != bsoncxx::type::k_array) {
        return "";
    }
    vector<string> tokens;
    for (const auto& token : item["tokens"].get_array().value) {
        if (token.type() == bsoncxx::type::k_string) {
            tokens.push_back(string(token.get_string().value));
        }
    }
    if (tokens.empty()) {
        return "";
    }
    return atomdb_api_types::AccessPermissionEntry(tokens, false, false).schema.handle();
}

bool MongoAuthorizationPersistence::read_full_access(bsoncxx::document::view view) {
    if (view["full_access"] && view["full_access"].type() == bsoncxx::type::k_bool) {
        return view["full_access"].get_bool().value;
    }
    return false;
}

void MongoAuthorizationPersistence::append_schemas_except(bsoncxx::builder::basic::array& schemas,
                                                          bsoncxx::document::view view,
                                                          const string& handle_to_skip) {
    if (!view["allowed_schemas"] || view["allowed_schemas"].type() != bsoncxx::type::k_array) {
        return;
    }
    for (const auto& item : view["allowed_schemas"].get_array().value) {
        if (item.type() != bsoncxx::type::k_document) {
            continue;
        }
        auto item_view = item.get_document().view();
        if (schema_handle_from_item(item_view) == handle_to_skip) {
            continue;
        }
        schemas.append(item_view);
    }
}

optional<bsoncxx::document::value> MongoAuthorizationPersistence::get_document_by_id(
    const string& id) {
    auto conn = this->pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];
    auto reply = collection.find_one(
        bsoncxx::v_noabi::builder::basic::make_document(bsoncxx::v_noabi::builder::basic::kvp("_id", id)));
}
