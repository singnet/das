#include "RedisMongoDB.h"

#include <algorithm>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include "Hasher.h"
#include "Link.h"
#include "Logger.h"
#include "Node.h"
#include "Properties.h"
#include "Utils.h"

using namespace atomdb;
using namespace commons;
using namespace atoms;

bool RedisMongoDB::SKIP_REDIS;
string RedisMongoDB::REDIS_PATTERNS_PREFIX;
string RedisMongoDB::REDIS_OUTGOING_PREFIX;
string RedisMongoDB::REDIS_INCOMING_PREFIX;
uint RedisMongoDB::REDIS_CHUNK_SIZE;
string RedisMongoDB::MONGODB_DB_NAME;
string RedisMongoDB::MONGODB_NODES_COLLECTION_NAME;
string RedisMongoDB::MONGODB_LINKS_COLLECTION_NAME;
string RedisMongoDB::MONGODB_PATTERN_INDEX_SCHEMA_COLLECTION_NAME;
string RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::size];
uint RedisMongoDB::MONGODB_CHUNK_SIZE;
mongocxx::instance RedisMongoDB::MONGODB_INSTANCE;

RedisMongoDB::RedisMongoDB(const string& context, bool skip_redis) {
    initialize_statics(context, skip_redis);
    mongodb_setup();
    load_pattern_index_schema();
    bool disable_cache = (Utils::get_environment("DAS_DISABLE_ATOMDB_CACHE") == "true");
    this->atomdb_cache = disable_cache ? nullptr : AtomDBCacheSingleton::get_instance();
    redis_setup();
    this->patterns_next_score.store(get_next_score(REDIS_PATTERNS_PREFIX + ":next_score"));
    this->incoming_set_next_score.store(get_next_score(REDIS_INCOMING_PREFIX + ":next_score"));
}

RedisMongoDB::~RedisMongoDB() {
    delete this->mongodb_pool;
    if (!SKIP_REDIS) delete this->redis_pool;
}

bool RedisMongoDB::allow_nested_indexing() { return false; }

void RedisMongoDB::redis_setup() {
    if (SKIP_REDIS) return;

    string host = Utils::get_environment("DAS_REDIS_HOSTNAME");
    string port = Utils::get_environment("DAS_REDIS_PORT");
    string address = host + ":" + port;
    string cluster = Utils::get_environment("DAS_USE_REDIS_CLUSTER");
    std::transform(cluster.begin(), cluster.end(), cluster.begin(), ::toupper);
    this->cluster_flag = (cluster == "TRUE");

    if (host == "" || port == "") {
        Utils::error(
            "You need to set Redis access info as environment variables: DAS_REDIS_HOSTNAME, "
            "DAS_REDIS_PORT and DAS_USE_REDIS_CLUSTER");
    }
    this->redis_pool = new RedisContextPool(this->cluster_flag, host, port, address);
    LOG_INFO("Connected to (" << (this->cluster_flag ? "CLUSTER" : "NON-CLUSTER") << ") Redis at "
                              << address);
}

void RedisMongoDB::mongodb_setup() {
    string host = Utils::get_environment("DAS_MONGODB_HOSTNAME");
    string port = Utils::get_environment("DAS_MONGODB_PORT");
    string user = Utils::get_environment("DAS_MONGODB_USERNAME");
    string password = Utils::get_environment("DAS_MONGODB_PASSWORD");
    try {
        uint chunk_size = Utils::string_to_int(Utils::get_environment("DAS_MONGODB_CHUNK_SIZE"));
        if (chunk_size > 0) {
            MONGODB_CHUNK_SIZE = chunk_size;
        }
    } catch (const std::exception& e) {
        LOG_INFO("Using default MongoDB chunk size: " + to_string(MONGODB_CHUNK_SIZE));
    }
    if (host == "" || port == "" || user == "" || password == "") {
        Utils::error(
            string("You need to set MongoDB access info as environment variables: ") +
            "DAS_MONGODB_HOSTNAME, DAS_MONGODB_PORT, DAS_MONGODB_USERNAME and DAS_MONGODB_PASSWORD");
    }
    string address = host + ":" + port;
    string url = "mongodb://" + user + ":" + password + "@" + address;

    try {
        auto uri = mongocxx::uri{url};
        this->mongodb_pool = new mongocxx::pool(uri);
        // Health check using ping command
        auto conn = this->mongodb_pool->acquire();
        auto mongodb = (*conn)[MONGODB_DB_NAME];
        const auto ping_cmd =
            bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1));
        mongodb.run_command(ping_cmd.view());
        LOG_INFO("Connected to MongoDB at " << address);
    } catch (const std::exception& e) {
        Utils::error(e.what());
    }
}

shared_ptr<Atom> RedisMongoDB::get_atom(const string& handle) {
    auto atom_document =
        dynamic_pointer_cast<atomdb_api_types::MongodbDocument>(get_atom_document(handle));
    if (atom_document != NULL) {
        Properties custom_attributes;
        if (atom_document->contains("custom_attributes")) {
            custom_attributes =
                atom_document->extract_custom_attributes(atom_document->get_object("custom_attributes"));
        }
        bool is_toplevel = false;
        if (atom_document->contains("is_toplevel")) {
            is_toplevel = atom_document->get_bool("is_toplevel");
        }
        if (atom_document->contains(MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS])) {
            unsigned int arity = atom_document->get_size(MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS]);
            vector<string> targets;
            for (unsigned int i = 0; i < arity; i++) {
                targets.push_back(
                    string(atom_document->get(MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS], i)));
            }
            return make_shared<Link>(
                atom_document->get("named_type"), targets, is_toplevel, custom_attributes);
        } else {
            return make_shared<Node>(atom_document->get("named_type"),
                                     atom_document->get("name"),
                                     is_toplevel,
                                     custom_attributes);
        }
    } else {
        return shared_ptr<Atom>(NULL);
    }
}

shared_ptr<atomdb_api_types::HandleSet> RedisMongoDB::query_for_pattern(const LinkSchema& link_schema) {
    if (SKIP_REDIS) return nullptr;

    auto pattern_handle = link_schema.handle();

    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_pattern(pattern_handle);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    unsigned int redis_cursor = 0;
    bool redis_has_more = true;
    string command;
    redisReply* reply;

    auto ctx = this->redis_pool->acquire();

    auto handle_set = make_shared<atomdb_api_types::HandleSetRedis>();

    while (redis_has_more) {
        command = ("ZRANGE " + REDIS_PATTERNS_PREFIX + ":" + pattern_handle + " " +
                   to_string(redis_cursor) + " " + to_string(redis_cursor + REDIS_CHUNK_SIZE - 1));

        reply = ctx->execute(command.c_str());

        if (reply == NULL) Utils::error("Redis error at query_for_pattern");

        if (reply->type != REDIS_REPLY_SET && reply->type != REDIS_REPLY_ARRAY) {
            auto error_type = std::to_string(reply->type);
            freeReplyObject(reply);
            Utils::error("Invalid Redis response at query_for_pattern: " + error_type);
        }

        redis_cursor += REDIS_CHUNK_SIZE;
        redis_has_more = (reply->elements == REDIS_CHUNK_SIZE);

        if (reply->elements == 0) {
            freeReplyObject(reply);
            break;
        }
        handle_set->append(make_shared<atomdb_api_types::HandleSetRedis>(reply, false));
    }

    if (this->atomdb_cache != nullptr)
        this->atomdb_cache->add_pattern_matching(pattern_handle, handle_set);
    return handle_set;
}

shared_ptr<atomdb_api_types::HandleList> RedisMongoDB::query_for_targets(const string& handle) {
    if (SKIP_REDIS) return nullptr;

    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_targets(handle);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    redisReply* reply;
    try {
        auto ctx = this->redis_pool->acquire();
        auto command = "GET " + REDIS_OUTGOING_PREFIX + ":" + handle;
        reply = ctx->execute(command.c_str());

        if (reply == NULL) Utils::error("Redis error at query_for_targets");

        if ((reply == NULL) || (reply->type == REDIS_REPLY_NIL)) {
            if (this->atomdb_cache != nullptr) this->atomdb_cache->add_handle_targets(handle, nullptr);
            return nullptr;
        }

        if (reply->type != REDIS_REPLY_STRING) {
            Utils::error("Invalid Redis response at query_for_targets: " + std::to_string(reply->type) +
                         " != " + std::to_string(REDIS_REPLY_STRING));
        }

    } catch (const std::exception& e) {
        Utils::error(e.what());
    }

    // NOTE: Intentionally, we aren't destroying 'reply' objects.'reply' objects are destroyed in
    // ~RedisSet().
    auto handle_list = make_shared<atomdb_api_types::RedisStringBundle>(reply);

    if (this->atomdb_cache != nullptr) this->atomdb_cache->add_handle_targets(handle, handle_list);
    return handle_list;
}

shared_ptr<atomdb_api_types::HandleSet> RedisMongoDB::query_for_incoming_set(const string& handle) {
    if (SKIP_REDIS) return nullptr;

    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_incoming_set(handle);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    unsigned int redis_cursor = 0;
    bool redis_has_more = true;
    string command;
    redisReply* reply;

    auto ctx = this->redis_pool->acquire();

    auto handle_set = make_shared<atomdb_api_types::HandleSetRedis>();

    while (redis_has_more) {
        command = ("ZRANGE " + REDIS_INCOMING_PREFIX + ":" + handle + " " + to_string(redis_cursor) +
                   " " + to_string(redis_cursor + REDIS_CHUNK_SIZE - 1));

        reply = ctx->execute(command.c_str());

        if (reply == NULL) Utils::error("Redis error at query_for_incoming_set");

        if (reply->type != REDIS_REPLY_SET && reply->type != REDIS_REPLY_ARRAY) {
            auto error_type = std::to_string(reply->type);
            freeReplyObject(reply);
            Utils::error("Invalid Redis response at query_for_incoming_set: " + error_type);
        }

        redis_cursor += REDIS_CHUNK_SIZE;
        redis_has_more = (reply->elements == REDIS_CHUNK_SIZE);

        if (reply->elements == 0) {
            freeReplyObject(reply);
            break;
        }
        handle_set->append(make_shared<atomdb_api_types::HandleSetRedis>(reply, false));
    }

    if (this->atomdb_cache != nullptr) this->atomdb_cache->add_incoming_set(handle, handle_set);

    return handle_set;
}

void RedisMongoDB::add_pattern(const string& pattern_handle, const string& handle) {
    if (SKIP_REDIS) return;

    auto ctx = this->redis_pool->acquire();
    string command = "ZADD " + REDIS_PATTERNS_PREFIX + ":" + pattern_handle + " " +
                     to_string(this->patterns_next_score.load()) + " " + handle;
    redisReply* reply = ctx->execute(command.c_str());
    if (reply == NULL) Utils::error("Redis error at add_pattern");

    if (reply->type != REDIS_REPLY_INTEGER) {
        Utils::error("Invalid Redis response at add_pattern: " + std::to_string(reply->type));
    }

    set_next_score(REDIS_PATTERNS_PREFIX + ":next_score", this->patterns_next_score.fetch_add(1));

    if (this->atomdb_cache != nullptr) this->atomdb_cache->erase_pattern_matching_cache(pattern_handle);
}

void RedisMongoDB::add_patterns(const vector<pair<string, string>>& pattern_handles) {
    if (SKIP_REDIS) return;

    if (pattern_handles.empty()) return;

    auto ctx = this->redis_pool->acquire();

    // Group patterns by pattern_handle for efficient batching
    map<string, vector<string>> patterns_by_handle;
    for (const auto& pair : pattern_handles) {
        patterns_by_handle[pair.first].push_back(pair.second);
    }

    // Use Redis pipeline for batch operations
    for (const auto& pattern_group : patterns_by_handle) {
        const string& pattern_handle = pattern_group.first;
        const vector<string>& handles = pattern_group.second;

        // Build ZADD command with multiple score-member pairs
        string command = "ZADD " + REDIS_PATTERNS_PREFIX + ":" + pattern_handle;
        for (const auto& handle : handles) {
            command += " " + to_string(this->patterns_next_score.load()) + " " + handle;
            this->patterns_next_score.fetch_add(1);
        }

        redisReply* reply = ctx->execute(command.c_str());
        if (reply == NULL) Utils::error("Redis error at add_patterns");

        if (reply->type != REDIS_REPLY_INTEGER) {
            Utils::error("Invalid Redis response at add_patterns: " + std::to_string(reply->type));
        }

        // Clear cache for this pattern
        if (this->atomdb_cache != nullptr) {
            this->atomdb_cache->erase_pattern_matching_cache(pattern_handle);
        }
    }
    set_next_score(REDIS_PATTERNS_PREFIX + ":next_score", this->patterns_next_score.load());
}

void RedisMongoDB::delete_pattern(const string& handle) {
    if (SKIP_REDIS) return;

    auto ctx = this->redis_pool->acquire();

    string command = "DEL " + REDIS_PATTERNS_PREFIX + ":" + handle;
    redisReply* reply = ctx->execute(command.c_str());

    if (reply == NULL) Utils::error("Redis error at delete_pattern");
    if (reply->type != REDIS_REPLY_INTEGER) {
        Utils::error("Invalid Redis response at delete_pattern: " + std::to_string(reply->type));
    }

    if (this->atomdb_cache != nullptr) this->atomdb_cache->erase_pattern_matching_cache(handle);
}

void RedisMongoDB::update_pattern(const string& key, const string& value) {
    if (SKIP_REDIS) return;

    auto ctx = this->redis_pool->acquire();
    string command = "ZREM " + REDIS_PATTERNS_PREFIX + ":" + key + " " + value;
    redisReply* reply = ctx->execute(command.c_str());
    if (reply == NULL) Utils::error("Redis error at update_pattern");

    if (this->atomdb_cache != nullptr) this->atomdb_cache->erase_pattern_matching_cache(key);
}

uint RedisMongoDB::get_next_score(const string& key) {
    if (SKIP_REDIS) return 0;

    auto ctx = this->redis_pool->acquire();
    string command = "GET " + key;
    redisReply* reply = ctx->execute(command.c_str());
    if (reply == NULL) Utils::error("Redis error at get_next_score");

    if (reply->type == REDIS_REPLY_NIL) return 0;

    if (reply->type != REDIS_REPLY_STRING) {
        Utils::error("Invalid Redis response at get_next_score: " + std::to_string(reply->type));
    }

    return std::stoi(reply->str);
}

void RedisMongoDB::set_next_score(const string& key, uint score) {
    if (SKIP_REDIS) return;

    auto ctx = this->redis_pool->acquire();
    string command = "SET " + key + " " + to_string(score);
    redisReply* reply = ctx->execute(command.c_str());
    if (reply == NULL) Utils::error("Redis error at set_next_score");

    if (reply->type != REDIS_REPLY_STATUS) {
        Utils::error("Invalid Redis response at set_next_score: " + std::to_string(reply->type));
    }
}

void RedisMongoDB::reset_scores() {
    this->patterns_next_score.store(get_next_score(REDIS_PATTERNS_PREFIX + ":next_score"));
    this->incoming_set_next_score.store(get_next_score(REDIS_INCOMING_PREFIX + ":next_score"));
}

void RedisMongoDB::add_outgoing_set(const string& handle, const vector<string>& outgoing_handles) {
    if (SKIP_REDIS) return;

    auto ctx = this->redis_pool->acquire();
    string command = "SET " + REDIS_OUTGOING_PREFIX + ":" + handle + " ";
    for (const auto& outgoing_handle : outgoing_handles) {
        command += outgoing_handle;
    }
    redisReply* reply = ctx->execute(command.c_str());
    if (reply == NULL) Utils::error("Redis error at add_outgoing_set");

    if (reply->type != REDIS_REPLY_STATUS) {
        Utils::error("Invalid Redis response at add_outgoing_set: " + std::to_string(reply->type));
    }
}

void RedisMongoDB::delete_outgoing_set(const string& handle) {
    if (SKIP_REDIS) return;

    auto ctx = this->redis_pool->acquire();
    string command = "DEL " + REDIS_OUTGOING_PREFIX + ":" + handle;
    redisReply* reply = ctx->execute(command.c_str());
    if (reply == NULL) Utils::error("Redis error at delete_outgoing_set");
    if (this->atomdb_cache != nullptr) this->atomdb_cache->erase_handle_targets_cache(handle);
}

void RedisMongoDB::add_incoming_set(const string& handle, const string& incoming_handle) {
    if (SKIP_REDIS) return;

    auto ctx = this->redis_pool->acquire();
    string command = "ZADD " + REDIS_INCOMING_PREFIX + ":" + handle + " " +
                     to_string(this->incoming_set_next_score.load()) + " " + incoming_handle;
    redisReply* reply = ctx->execute(command.c_str());
    if (reply == NULL) Utils::error("Redis error at add_incoming_set");

    if (reply->type != REDIS_REPLY_INTEGER) {
        Utils::error("Invalid Redis response at add_incoming_set: " + std::to_string(reply->type));
    }

    set_next_score(REDIS_INCOMING_PREFIX + ":next_score", this->incoming_set_next_score.fetch_add(1));

    if (this->atomdb_cache != nullptr) this->atomdb_cache->erase_incoming_set_cache(handle);
}

void RedisMongoDB::delete_incoming_set(const string& handle) {
    if (SKIP_REDIS) return;

    auto ctx = this->redis_pool->acquire();

    string command = "DEL " + REDIS_INCOMING_PREFIX + ":" + handle;
    redisReply* reply = ctx->execute(command.c_str());

    if (reply == NULL) Utils::error("Redis error at delete_incoming_set");
    if (reply->type != REDIS_REPLY_INTEGER) {
        Utils::error("Invalid Redis response at delete_incoming_set: " + std::to_string(reply->type));
    }

    if (this->atomdb_cache != nullptr) this->atomdb_cache->erase_incoming_set_cache(handle);
}

void RedisMongoDB::update_incoming_set(const string& key, const string& value) {
    if (SKIP_REDIS) return;

    auto ctx = this->redis_pool->acquire();

    string command = "ZREM " + REDIS_INCOMING_PREFIX + ":" + key + " " + value;
    redisReply* reply = ctx->execute(command.c_str());

    if (reply == NULL) Utils::error("Redis error at update_incoming_set");
    if (reply->type != REDIS_REPLY_INTEGER) {
        Utils::error("Invalid Redis response at update_incoming_set: " + std::to_string(reply->type));
    }

    if (this->atomdb_cache != nullptr) this->atomdb_cache->erase_incoming_set_cache(key);
}

shared_ptr<atomdb_api_types::AtomDocument> RedisMongoDB::get_document(const string& handle,
                                                                      const string& collection_name) {
    if (this->atomdb_cache != nullptr) {
        if (collection_name == MONGODB_NODES_COLLECTION_NAME) {
            auto cache_result = this->atomdb_cache->get_node_document(handle);
            if (cache_result.is_cache_hit) return cache_result.result;
        } else if (collection_name == MONGODB_LINKS_COLLECTION_NAME) {
            auto cache_result = this->atomdb_cache->get_link_document(handle);
            if (cache_result.is_cache_hit) return cache_result.result;
        }
    }

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][collection_name];
    auto reply = mongodb_collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], handle)));

    auto atom_document = reply != bsoncxx::v_noabi::stdx::nullopt
                             ? make_shared<atomdb_api_types::MongodbDocument>(reply)
                             : nullptr;

    if (this->atomdb_cache != nullptr && atom_document != nullptr) {
        if (collection_name == MONGODB_NODES_COLLECTION_NAME) {
            this->atomdb_cache->add_node_document(handle, atom_document);
        } else if (collection_name == MONGODB_LINKS_COLLECTION_NAME) {
            this->atomdb_cache->add_link_document(handle, atom_document);
        }
    }

    return atom_document;
}

shared_ptr<atomdb_api_types::AtomDocument> RedisMongoDB::get_atom_document(const string& handle) {
    auto node_document = get_node_document(handle);
    if (node_document != nullptr) {
        return node_document;
    }
    auto link_document = get_link_document(handle);
    if (link_document != nullptr) {
        return link_document;
    }
    return nullptr;
}

shared_ptr<atomdb_api_types::AtomDocument> RedisMongoDB::get_node_document(const string& handle) {
    return get_document(handle, MONGODB_NODES_COLLECTION_NAME);
}

shared_ptr<atomdb_api_types::AtomDocument> RedisMongoDB::get_link_document(const string& handle) {
    return get_document(handle, MONGODB_LINKS_COLLECTION_NAME);
}

vector<shared_ptr<atomdb_api_types::AtomDocument>> RedisMongoDB::get_filtered_documents(
    const string& collection_name,
    const bsoncxx::builder::stream::document& filter_builder,
    const vector<string>& fields) {
    vector<shared_ptr<atomdb_api_types::AtomDocument>> atom_documents;

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][collection_name];

    try {
        // Fetch documents in batches
        uint skip = 0;
        bool has_more = true;
        while (has_more) {
            // Build projection
            bsoncxx::builder::stream::document projection_builder;
            for (const auto& field : fields) {
                projection_builder << field << 1;
            }

            auto cursor = mongodb_collection.find(filter_builder.view(),
                                                  mongocxx::options::find{}
                                                      .projection(projection_builder.view())
                                                      .skip(skip)
                                                      .limit(MONGODB_CHUNK_SIZE));

            uint documents_count = 0;
            for (const auto& view : cursor) {
                bsoncxx::v_noabi::stdx::optional<bsoncxx::v_noabi::document::value> opt_val =
                    bsoncxx::v_noabi::document::value(view);
                auto atom_document = make_shared<atomdb_api_types::MongodbDocument>(opt_val);
                atom_documents.push_back(atom_document);
                documents_count++;
            }

            has_more = documents_count == MONGODB_CHUNK_SIZE;

            skip += MONGODB_CHUNK_SIZE;
        }
    } catch (const exception& e) {
        Utils::error("MongoDB error at get_filtered_documents(): " + string(e.what()));
    }

    return atom_documents;
}

vector<shared_ptr<atomdb_api_types::AtomDocument>> RedisMongoDB::get_documents(
    const vector<string>& handles, const vector<string>& fields, const string& collection_name) {
    // TODO Add cache support for this method
    vector<shared_ptr<atomdb_api_types::AtomDocument>> atom_documents;

    if (handles.empty()) {
        return atom_documents;
    }

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][collection_name];

    try {
        // Process handles in batches
        uint handle_count = handles.size();
        for (size_t i = 0; i < handles.size(); i += MONGODB_CHUNK_SIZE) {
            size_t batch_size = min(MONGODB_CHUNK_SIZE, uint(handle_count - i));
            // Build filter
            bsoncxx::builder::stream::document filter_builder;
            auto array = filter_builder << MONGODB_FIELD_NAME[MONGODB_FIELD::ID]
                                        << bsoncxx::builder::stream::open_document << "$in"
                                        << bsoncxx::builder::stream::open_array;
            for (size_t j = i; j < (i + batch_size); j++) {
                array << handles[j];
            }
            array << bsoncxx::builder::stream::close_array << bsoncxx::builder::stream::close_document;

            // Build projection
            bsoncxx::builder::stream::document projection_builder;
            for (const auto& field : fields) {
                projection_builder << field << 1;
            }

            auto cursor = mongodb_collection.find(
                filter_builder.view(), mongocxx::options::find{}.projection(projection_builder.view()));

            for (const auto& view : cursor) {
                bsoncxx::v_noabi::stdx::optional<bsoncxx::v_noabi::document::value> opt_val =
                    bsoncxx::v_noabi::document::value(view);
                auto atom_document = make_shared<atomdb_api_types::MongodbDocument>(opt_val);
                atom_documents.push_back(atom_document);
            }
        }
    } catch (const exception& e) {
        Utils::error("MongoDB error: " + string(e.what()));
    }

    return atom_documents;
}

vector<shared_ptr<atomdb_api_types::AtomDocument>> RedisMongoDB::get_atom_documents(
    const vector<string>& handles, const vector<string>& fields) {
    auto documents = get_node_documents(handles, fields);
    if (documents.size() == handles.size()) return documents;

    vector<string> missing_handles = handles;
    for (const auto& document : documents) {
        auto handle = document->get(MONGODB_FIELD_NAME[MONGODB_FIELD::ID]);
        missing_handles.erase(find(missing_handles.begin(), missing_handles.end(), handle));
    }

    auto link_documents = get_link_documents(missing_handles, fields);
    documents.insert(documents.end(), link_documents.begin(), link_documents.end());
    return documents;
}

vector<shared_ptr<atomdb_api_types::AtomDocument>> RedisMongoDB::get_node_documents(
    const vector<string>& handles, const vector<string>& fields) {
    return get_documents(handles, fields, MONGODB_NODES_COLLECTION_NAME);
}

vector<shared_ptr<atomdb_api_types::AtomDocument>> RedisMongoDB::get_link_documents(
    const vector<string>& handles, const vector<string>& fields) {
    return get_documents(handles, fields, MONGODB_LINKS_COLLECTION_NAME);
}

vector<shared_ptr<atomdb_api_types::AtomDocument>> RedisMongoDB::get_matching_atoms(bool is_toplevel,
                                                                                    Atom& key) {
    vector<shared_ptr<atomdb_api_types::AtomDocument>> matching_atoms;

    vector<string> collection_names = {MONGODB_NODES_COLLECTION_NAME, MONGODB_LINKS_COLLECTION_NAME};
    bool add_id_filter = false;
    if (dynamic_cast<atoms::Link*>(&key)) {
        collection_names = {MONGODB_LINKS_COLLECTION_NAME};
        add_id_filter = true;
    } else if (dynamic_cast<atoms::Node*>(&key)) {
        collection_names = {MONGODB_NODES_COLLECTION_NAME};
        add_id_filter = true;
    }

    for (const auto& collection_name : collection_names) {
        bsoncxx::builder::stream::document filter_builder;

        if (add_id_filter) {
            filter_builder << MONGODB_FIELD_NAME[MONGODB_FIELD::ID] << key.handle();
        }

        // TODO(arturgontijo): Properly add support for Node's is_toplevel filter
        if (collection_name == MONGODB_LINKS_COLLECTION_NAME || is_toplevel) {
            filter_builder << "is_toplevel" << is_toplevel;
        }

        auto documents = get_filtered_documents(collection_name, filter_builder, {});
        for (const auto& document : documents) {
            Assignment assignment;
            if (key.match(document->get(MONGODB_FIELD_NAME[MONGODB_FIELD::ID]), assignment, *this)) {
                matching_atoms.push_back(document);
            }
        }
    }

    return matching_atoms;
}

bool RedisMongoDB::document_exists(const string& handle, const string& collection_name) {
    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][collection_name];
    auto reply = mongodb_collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], handle)));
    return reply != bsoncxx::v_noabi::stdx::nullopt;
}

bool RedisMongoDB::atom_exists(const string& atom_handle) {
    return node_exists(atom_handle) || link_exists(atom_handle);
}

bool RedisMongoDB::node_exists(const string& node_handle) {
    return document_exists(node_handle, MONGODB_NODES_COLLECTION_NAME);
}

bool RedisMongoDB::link_exists(const string& link_handle) {
    return document_exists(link_handle, MONGODB_LINKS_COLLECTION_NAME);
}

set<string> RedisMongoDB::documents_exist(const vector<string>& handles, const string& collection_name) {
    if (handles.empty()) return {};

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][collection_name];

    set<string> existing_handles;
    try {
        // Process handles in batches
        uint handle_count = handles.size();
        for (size_t i = 0; i < handles.size(); i += MONGODB_CHUNK_SIZE) {
            size_t batch_size = min(MONGODB_CHUNK_SIZE, uint(handle_count - i));
            // Build filter
            bsoncxx::builder::stream::document filter_builder;
            auto array = filter_builder << MONGODB_FIELD_NAME[MONGODB_FIELD::ID]
                                        << bsoncxx::builder::stream::open_document << "$in"
                                        << bsoncxx::builder::stream::open_array;
            for (size_t j = i; j < (i + batch_size); j++) {
                array << handles[j];
            }
            array << bsoncxx::builder::stream::close_array << bsoncxx::builder::stream::close_document;

            auto cursor = mongodb_collection.find(filter_builder.view());

            for (const auto& view : cursor) {
                existing_handles.insert(
                    view[MONGODB_FIELD_NAME[MONGODB_FIELD::ID]].get_string().value.data());
            }
        }
    } catch (const exception& e) {
        Utils::error("MongoDB error: " + string(e.what()));
    }

    return existing_handles;
}

set<string> RedisMongoDB::atoms_exist(const vector<string>& handles) {
    auto nodes = nodes_exist(handles);
    if (nodes.size() == handles.size()) return nodes;
    auto links = links_exist(handles);
    nodes.insert(links.begin(), links.end());
    return nodes;
}

set<string> RedisMongoDB::nodes_exist(const vector<string>& node_handles) {
    return documents_exist(node_handles, MONGODB_NODES_COLLECTION_NAME);
}

set<string> RedisMongoDB::links_exist(const vector<string>& link_handles) {
    return documents_exist(link_handles, MONGODB_LINKS_COLLECTION_NAME);
}

string RedisMongoDB::add_atom(const atoms::Atom* atom) {
    if (atom->arity() == 0) {
        return add_node(dynamic_cast<const atoms::Node*>(atom));
    } else {
        return add_link(dynamic_cast<const atoms::Link*>(atom));
    }
}

string RedisMongoDB::add_node(const atoms::Node* node) {
    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_NODES_COLLECTION_NAME];

    auto mongodb_doc = atomdb_api_types::MongodbDocument(node);
    auto reply = mongodb_collection.insert_one(mongodb_doc.value());

    if (!reply) {
        Utils::error("Failed to insert node into MongoDB");
    }

    return node->handle();
}

string RedisMongoDB::add_link(const atoms::Link* link) {
    auto existing_targets = get_atom_documents(link->targets, {MONGODB_FIELD_NAME[MONGODB_FIELD::ID]});
    if (existing_targets.size() != link->targets.size()) {
        Utils::error("Failed to insert link: " + link->handle() + " has " +
                     to_string(link->targets.size() - existing_targets.size()) + " missing targets");
        return "";
    }

    auto pattern_handles = match_pattern_index_schema(link);
    for (const auto& pattern_handle : pattern_handles) {
        add_pattern(pattern_handle, link->handle());
    }

    for (const auto& target : existing_targets) {
        add_incoming_set(target->get(MONGODB_FIELD_NAME[MONGODB_FIELD::ID]), link->handle());
    }

    add_outgoing_set(link->handle(), link->targets);

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_LINKS_COLLECTION_NAME];

    auto mongodb_doc = atomdb_api_types::MongodbDocument(link, *this);
    auto reply = mongodb_collection.insert_one(mongodb_doc.value());

    if (!reply) {
        Utils::error("Failed to insert link into MongoDB");
    }

    return link->handle();
}

vector<string> RedisMongoDB::add_atoms(const vector<atoms::Atom*>& atoms) {
    if (atoms.empty()) {
        return {};
    }

    vector<Node*> nodes;
    vector<Link*> links;
    for (const auto& atom : atoms) {
        if (atom->arity() == 0) {
            nodes.push_back(dynamic_cast<atoms::Node*>(atom));
        } else {
            links.push_back(dynamic_cast<atoms::Link*>(atom));
        }
    }
    auto node_handles = add_nodes(nodes);
    auto link_handles = add_links(links);
    node_handles.insert(node_handles.end(), link_handles.begin(), link_handles.end());
    return node_handles;
}

vector<string> RedisMongoDB::add_nodes(const vector<atoms::Node*>& nodes) {
    if (nodes.empty()) {
        return {};
    }

    vector<bsoncxx::v_noabi::document::value> docs;
    vector<string> handles;

    for (const auto& node : nodes) {
        auto mongodb_doc = atomdb_api_types::MongodbDocument(node);
        handles.push_back(node->handle());
        docs.push_back(mongodb_doc.value());
    }

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_NODES_COLLECTION_NAME];

    auto reply = mongodb_collection.insert_many(docs);

    if (!reply) {
        Utils::error("Failed to insert nodes into MongoDB");
    }

    return handles;
}

vector<string> RedisMongoDB::add_links(const vector<atoms::Link*>& links) {
    if (links.empty()) {
        return {};
    }
    vector<string> handles;
    for (const auto& link : links) {
        handles.push_back(add_link(link));
    }
    return handles;
}

bool RedisMongoDB::delete_document(const string& handle,
                                   const string& collection_name,
                                   bool delete_targets) {
    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][collection_name];
    auto reply = mongodb_collection.delete_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], handle)));

    if (this->atomdb_cache != nullptr) {
        if (collection_name == MONGODB_NODES_COLLECTION_NAME) {
            this->atomdb_cache->erase_node_document_cache(handle);
        } else if (collection_name == MONGODB_LINKS_COLLECTION_NAME) {
            this->atomdb_cache->erase_link_document_cache(handle);
        }
    }

    if (!SKIP_REDIS) {
        auto incoming_set = query_for_incoming_set(handle);
        auto it = incoming_set->get_iterator();
        char* incoming_handle;
        while ((incoming_handle = it->next()) != nullptr) {
            delete_atom(incoming_handle, delete_targets);
        }

        delete_incoming_set(handle);

        delete_outgoing_set(handle);
    }

    // NOTE: the initial handle might be already deleted due the recursive delete_atom() calls.
    return reply->deleted_count() > 0 || !document_exists(handle, collection_name);
}

bool RedisMongoDB::delete_atom(const string& handle, bool delete_targets) {
    if (delete_node(handle, delete_targets)) return true;
    return delete_link(handle, delete_targets);
}

bool RedisMongoDB::delete_node(const string& handle, bool delete_targets) {
    auto node_document = get_node_document(handle);
    if (node_document == nullptr) return false;
    return delete_document(handle, MONGODB_NODES_COLLECTION_NAME, delete_targets);
}

bool RedisMongoDB::delete_link(const string& handle, bool delete_targets) {
    auto link_document = get_link_document(handle);
    if (link_document == nullptr) return false;

    vector<string> targets;
    auto targets_size = link_document->get_size(MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS]);
    for (uint i = 0; i < targets_size; i++) {
        auto target_handle = link_document->get(MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS], i);
        // If target is referenced more than once, we need to update incoming_set or delete target
        // otherwise
        if (!SKIP_REDIS && query_for_incoming_set(target_handle)->size() > 1) {
            update_incoming_set(target_handle, handle);
        } else if (delete_targets) {
            delete_atom(target_handle, delete_targets);
        }
        targets.push_back(target_handle);
    }

    auto link = new Link(link_document->get(MONGODB_FIELD_NAME[MONGODB_FIELD::NAMED_TYPE]), targets);
    auto pattern_handles = match_pattern_index_schema(link);
    for (const auto& pattern_handle : pattern_handles) {
        update_pattern(pattern_handle, link->handle());
    }

    return delete_document(handle, MONGODB_LINKS_COLLECTION_NAME, delete_targets);
}

uint RedisMongoDB::delete_documents(const vector<string>& handles,
                                    const string& collection_name,
                                    bool delete_targets) {
    uint deleted_count = 0;
    for (const auto& handle : handles) {
        if (delete_document(handle, collection_name, delete_targets)) {
            deleted_count++;
        }
    }
    return deleted_count;
}

uint RedisMongoDB::delete_atoms(const vector<string>& handles, bool delete_targets) {
    uint deleted_count = 0;
    for (const auto& handle : handles) {
        if (delete_atom(handle, delete_targets)) {
            deleted_count++;
        }
    }
    return deleted_count;
}

uint RedisMongoDB::delete_nodes(const vector<string>& handles, bool delete_targets) {
    return delete_documents(handles, MONGODB_NODES_COLLECTION_NAME, delete_targets);
}

uint RedisMongoDB::delete_links(const vector<string>& handles, bool delete_targets) {
    uint deleted_count = 0;
    for (const auto& handle : handles) {
        if (delete_link(handle, delete_targets)) {
            deleted_count++;
        }
    }
    return deleted_count;
}

void RedisMongoDB::add_pattern_index_schema(const string& tokens,
                                            const vector<vector<string>>& index_entries) {
    auto tokens_vector = Utils::split(tokens, ' ');
    LinkSchema link_schema(tokens_vector);
    auto id = link_schema.handle();

    auto index_entries_array = bsoncxx::builder::basic::array{};
    for (const auto& index_entry : index_entries) {
        auto index_entry_array = bsoncxx::builder::basic::array{};
        for (const auto& index_entry_item : index_entry) {
            index_entry_array.append(index_entry_item);
        }
        index_entries_array.append(index_entry_array);
    }

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_PATTERN_INDEX_SCHEMA_COLLECTION_NAME];

    auto reply = mongodb_collection.insert_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], id),
        bsoncxx::v_noabi::builder::basic::kvp("tokens", tokens),
        bsoncxx::v_noabi::builder::basic::kvp("priority", this->pattern_index_schema_next_priority),
        bsoncxx::v_noabi::builder::basic::kvp("index_entries", index_entries_array)));

    if (!reply) {
        Utils::error("Failed to insert pattern index schema into MongoDB");
    }

    this->pattern_index_schema_map[this->pattern_index_schema_next_priority] =
        make_tuple(move(tokens_vector), index_entries);
    this->pattern_index_schema_next_priority++;
}

void RedisMongoDB::load_pattern_index_schema() {
    this->pattern_index_schema_map.clear();
    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_PATTERN_INDEX_SCHEMA_COLLECTION_NAME];
    auto cursor = mongodb_collection.find({});
    for (const auto& view : cursor) {
        // Extract _id
        string id = view["_id"].get_string().value.data();
        // Extract LinkSchema tokens
        vector<string> tokens = Utils::split(view["tokens"].get_string().value.data(), ' ');
        // Extract index_entries
        vector<vector<string>> index_entries;
        for (const auto& arr : view["index_entries"].get_array().value) {
            vector<string> entry;
            for (const auto& elem : arr.get_array().value) {
                entry.push_back(elem.get_string().value.data());
            }
            index_entries.push_back(entry);
        }

        auto priority = view["priority"].get_int32().value;
        this->pattern_index_schema_map[priority] = make_tuple(move(tokens), move(index_entries));
        this->pattern_index_schema_next_priority =
            fmax(this->pattern_index_schema_next_priority, priority + 1);
    }

    if (this->pattern_index_schema_map.size() == 0) {
        LOG_INFO(
            "WARNING: No pattern_index_schema found, all possible patterns will be created during link "
            "insertion!");
    }
}

vector<string> RedisMongoDB::match_pattern_index_schema(const Link* link) {
    vector<string> pattern_handles;
    auto local_map = this->pattern_index_schema_map;

    if (local_map.size() == 0) {
        vector<string> tokens = {"LINK_TEMPLATE", "Expression", to_string(link->arity())};
        for (unsigned int i = 0; i < link->arity(); i++) {
            tokens.push_back("VARIABLE");
            tokens.push_back("v" + to_string(i + 1));
        }

        auto link_schema = LinkSchema(tokens);
        auto index_entries = index_entries_combinations(link->arity());

        local_map[1] = make_tuple(move(tokens), move(index_entries));
    }

    vector<int> sorted_keys;
    for (const auto& pair : local_map) {
        sorted_keys.push_back(pair.first);
    }
    std::sort(sorted_keys.begin(), sorted_keys.end(), std::greater<int>());

    for (const auto& priority : sorted_keys) {
        auto value = local_map[priority];
        auto link_schema = LinkSchema(get<0>(value));
        auto index_entries = get<1>(value);
        Assignment assignment;
        bool match = link_schema.match(*(Link*) link, assignment, *this);
        if (match) {
            for (const auto& index_entry : index_entries) {
                size_t index = 0;
                vector<string> hash_entries;
                for (const auto& token : index_entry) {
                    if (token == "_") {
                        hash_entries.push_back(link->targets[index]);
                    } else if (token == "*") {
                        hash_entries.push_back(Atom::WILDCARD_STRING);
                    } else {
                        string assignment_value = assignment.get(token);
                        if (assignment_value == "") {
                            Utils::error("LinkSchema assignments don't have variable: " + token);
                        }
                        hash_entries.push_back(assignment_value);
                    }
                    index++;
                }
                string hash = Hasher::link_handle(link->type, hash_entries);
                pattern_handles.push_back(hash);
            }
            // We only need to find the first match
            break;
        }
    }
    return pattern_handles;
}

// Combination of "vX" and "*" for a given arity
vector<vector<string>> RedisMongoDB::index_entries_combinations(unsigned int arity) {
    vector<vector<string>> index_entries;
    unsigned int total = 1 << arity;  // 2^arity

    for (unsigned int mask = 0; mask < total; ++mask) {
        vector<string> index_entry;
        for (unsigned int i = 0; i < arity; ++i) {
            if (mask & (1 << i))
                index_entry.push_back("*");
            else
                index_entry.push_back("v" + to_string(i + 1));
        }
        index_entries.push_back(index_entry);
    }

    return index_entries;
}

void RedisMongoDB::re_index_patterns() {
    vector<Link*> links;
    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_LINKS_COLLECTION_NAME];
    auto cursor = mongodb_collection.find({});
    for (const auto& view : cursor) {
        vector<string> targets;
        for (const auto& target : view["targets"].get_array().value) {
            targets.push_back(target.get_string().value.data());
        }
        links.push_back(new Link(view["named_type"].get_string().value.data(), targets));
    }

    // Load pattern index schema
    load_pattern_index_schema();

    // Flush Redis patterns indexes
    flush_redis_by_prefix(REDIS_PATTERNS_PREFIX);

    // Reset patterns next score
    this->patterns_next_score.store(1);
    set_next_score(REDIS_PATTERNS_PREFIX + ":next_score", this->patterns_next_score.load());

    // Re-index patterns using batch insertion
    unsigned int count = 0;
    vector<pair<string, string>> all_pattern_pairs;
    for (const auto& link : links) {
        auto pattern_handles = match_pattern_index_schema(link);
        count++;
        for (const auto& pattern_handle : pattern_handles) {
            all_pattern_pairs.push_back({pattern_handle, link->handle()});
            if (all_pattern_pairs.size() >= REDIS_CHUNK_SIZE * 5) {
                LOG_INFO("Adding " + to_string(count) + "/" + to_string(links.size()) +
                         " links to patterns in Redis");
                add_patterns(all_pattern_pairs);
                all_pattern_pairs.clear();
            }
        }
    }

    if (!all_pattern_pairs.empty()) {
        LOG_INFO("Adding " + to_string(count) + "/" + to_string(links.size()) +
                 " links to patterns in Redis");
        add_patterns(all_pattern_pairs);
    }
}

void RedisMongoDB::flush_redis_by_prefix(const string& prefix) {
    if (SKIP_REDIS) return;

    auto ctx = this->redis_pool->acquire();
    std::string cursor = "0";
    do {
        std::string scan_cmd =
            "SCAN " + cursor + " MATCH " + prefix + ":* COUNT " + to_string(REDIS_CHUNK_SIZE);
        redisReply* reply = ctx->execute(scan_cmd.c_str());
        if (reply == NULL || reply->type != REDIS_REPLY_ARRAY || reply->elements < 2) {
            Utils::error("Redis error at flush_redis_by_prefix");
        }
        cursor = reply->element[0]->str;
        redisReply* keys = reply->element[1];
        if (keys->type == REDIS_REPLY_ARRAY && keys->elements > 0) {
            std::string del_cmd = "DEL";
            for (size_t i = 0; i < keys->elements; ++i) {
                del_cmd += " ";
                del_cmd += keys->element[i]->str;
            }
            redisReply* del_reply = ctx->execute(del_cmd.c_str());
            if (del_reply == NULL) {
                Utils::error("Redis error at flush_redis_by_prefix");
            }
            freeReplyObject(del_reply);
        }
        freeReplyObject(reply);
    } while (cursor != "0");

    if (this->atomdb_cache != nullptr) {
        if (prefix == REDIS_PATTERNS_PREFIX) {
            this->atomdb_cache->clear_all_pattern_handles();
        } else if (prefix == REDIS_OUTGOING_PREFIX) {
            this->atomdb_cache->clear_all_targets_handles();
        } else if (prefix == REDIS_INCOMING_PREFIX) {
            this->atomdb_cache->clear_all_incoming_handles();
        }
    }
}

void RedisMongoDB::drop_all() {
    // Drop MongoDB database
    auto conn = this->mongodb_pool->acquire();
    (*conn)[MONGODB_DB_NAME].drop();

    // Drop Redis database (by prefixes)
    if (!SKIP_REDIS) {
        auto ctx = this->redis_pool->acquire();
        vector<string> keys = {REDIS_PATTERNS_PREFIX, REDIS_OUTGOING_PREFIX, REDIS_INCOMING_PREFIX};
        for (const auto& key : keys) {
            flush_redis_by_prefix(key);
        }

        // Flushing next_scores from Redis and reseting them
        keys = {REDIS_PATTERNS_PREFIX, REDIS_INCOMING_PREFIX};
        for (const auto& key : keys) {
            flush_redis_by_prefix(key + ":next_score");
        }
        reset_scores();
    }

    // Reset next_priority
    this->pattern_index_schema_next_priority = 1;

    // We need to clear the pattern index schema map and reload it
    this->pattern_index_schema_map.clear();
    this->load_pattern_index_schema();
}
