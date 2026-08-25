#include "MongodbAuthorizationPersistence.h"

#include "Hasher.h"
#include "JsonConfig.h"
#include "MongoInitializer.h"
#include "MongodbAuthorizationProfile.h"
#include "Utils.h"
#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace atomdb_api_types;
using namespace commons;

namespace {

vector<string> tokens_from_entry(const AccessPermissionEntry& entry) {
    vector<string> tokens;
    tokens.reserve(entry.get_tokens_size());
    for (unsigned int i = 0; i < entry.get_tokens_size(); ++i) {
        tokens.push_back(entry.get_token(i));
    }
    return tokens;
}

}  // namespace

// --------------------------------------------------------------------------------
// Constructors

MongodbAuthorizationPersistence::MongodbAuthorizationPersistence(const string& endpoint,
                                                                 const string& username,
                                                                 const string& password,
                                                                 const string& database_name,
                                                                 const string& collection_name)
    : database_name(database_name), collection_name(collection_name) {
    if (endpoint.empty() || endpoint == ":" || username.empty() || password.empty()) {
        RAISE_ERROR("Invalid MongoDB configuration: need non-empty username, username, and password.");
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
    return schemas_from_document(*document);
}

void MongodbAuthorizationPersistence::save(const string& public_key, const AuthorizationSchema& entry) {
    auto conn = this->mongodb_pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];

    auto access_document = this->get_document(collection, public_key);

    MongodbAuthorizationProfile profile;
    if (access_document) {
        profile.set_access_key(access_document->get_access_key());
        profile.set_full_access(access_document->get_full_access());

        bool entry_exists = false;
        for (unsigned int i = 0; i < access_document->get_entries_size(); ++i) {
            const auto& document_entry = access_document->get_entry(i);
            if (LinkSchema(tokens_from_entry(document_entry)).handle() == entry.schema().handle()) {
                profile.append_entry(LinkSchema(entry.schema()).tokenize(), entry.read(), entry.write());
                entry_exists = true;
            } else {
                profile.append_entry(tokens_from_entry(document_entry),
                                     document_entry.get_read(),
                                     document_entry.get_write());
            }
        }

        if (!entry_exists) {
            profile.append_entry(LinkSchema(entry.schema()).tokenize(), entry.read(), entry.write());
        }
    } else {
        profile.set_access_key(public_key);
        profile.set_full_access(false);
        profile.append_entry(LinkSchema(entry.schema()).tokenize(), entry.read(), entry.write());
    }

    auto filter = bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp("_id", Hasher::plain_string_hash(profile.get_access_key())));

    mongocxx::options::replace opts;
    opts.upsert(true);

    auto reply = collection.replace_one(filter.view(), profile.value().view(), opts);

    if (!reply) {
        RAISE_ERROR("Failed to update authorization entry in MongoDB");
    }
}

void MongodbAuthorizationPersistence::remove(const string& public_key,
                                             const AuthorizationSchema& entry) {
    auto conn = this->mongodb_pool->acquire();
    auto collection = (*conn)[this->database_name][this->collection_name];

    auto access_document = this->get_document(collection, public_key);

    if (!access_document) return;

    MongodbAuthorizationProfile profile;
    profile.set_access_key(access_document->get_access_key());
    profile.set_full_access(access_document->get_full_access());

    bool has_schemas = false;
    for (unsigned int i = 0; i < access_document->get_entries_size(); ++i) {
        const auto& document_entry = access_document->get_entry(i);
        if (LinkSchema(tokens_from_entry(document_entry)).handle() == entry.schema().handle()) {
            continue;
        }
        profile.append_entry(
            tokens_from_entry(document_entry), document_entry.get_read(), document_entry.get_write());
        has_schemas = true;
    }

    if (!has_schemas) {
        this->remove_all(public_key);
        return;
    }

    auto filter = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp(
        "_id", Hasher::plain_string_hash(access_document->get_access_key())));

    auto reply = collection.replace_one(filter.view(), profile.value().view());

    if (!reply) {
        RAISE_ERROR("Failed to update authorization entry in MongoDB");
    }
}

void MongodbAuthorizationPersistence::remove_all(const string& public_key) {
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

vector<AuthorizationSchema> MongodbAuthorizationPersistence::schemas_from_document(
    const AccessPermissionDocument& document) {
    vector<AuthorizationSchema> schemas;
    schemas.reserve(document.get_entries_size());
    for (unsigned int i = 0; i < document.get_entries_size(); ++i) {
        const auto& entry = document.get_entry(i);
        schemas.emplace_back(tokens_from_entry(entry), entry.get_read(), entry.get_write());
    }
    return schemas;
}

shared_ptr<MongodbAccessPermissionDocument> MongodbAuthorizationPersistence::get_document(
    mongocxx::collection& collection, const string& public_key) {
    auto reply = collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp("_id", Hasher::plain_string_hash(public_key))));

    if (!reply) return nullptr;

    return make_shared<MongodbAccessPermissionDocument>(reply.value());
}
