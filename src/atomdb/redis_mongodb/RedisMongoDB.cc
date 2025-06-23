#include "RedisMongoDB.h"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "AttentionBrokerServer.h"
#include "Logger.h"
#include "Utils.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"

using namespace atomdb;
using namespace commons;

string RedisMongoDB::REDIS_PATTERNS_PREFIX;
string RedisMongoDB::REDIS_TARGETS_PREFIX;
uint RedisMongoDB::REDIS_CHUNK_SIZE;
string RedisMongoDB::MONGODB_DB_NAME;
string RedisMongoDB::MONGODB_COLLECTION_NAME;
string RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::size];
uint RedisMongoDB::MONGODB_CHUNK_SIZE;

RedisMongoDB::RedisMongoDB() {
    redis_setup();
    mongodb_setup();
    attention_broker_setup();
    bool disable_cache = (Utils::get_environment("DAS_DISABLE_ATOMDB_CACHE") == "true");
    this->atomdb_cache = disable_cache ? nullptr : AtomDBCacheSingleton::get_instance();
}

RedisMongoDB::~RedisMongoDB() {
    delete this->mongodb_pool;
    delete this->redis_pool;
}

void RedisMongoDB::attention_broker_setup() {
    grpc::ClientContext context;
    grpc::Status status;
    dasproto::Empty empty;
    dasproto::Ack ack;
    string attention_broker_address = Utils::get_environment("DAS_ATTENTION_BROKER_ADDRESS");
    string attention_broker_port = Utils::get_environment("DAS_ATTENTION_BROKER_PORT");

    if (attention_broker_address.empty()) {
        attention_broker_address = "localhost";
    }
    if (attention_broker_port.empty()) {
        attention_broker_address += ":37007";
    } else {
        attention_broker_address += ":" + attention_broker_port;
    }

    auto stub = dasproto::AttentionBroker::NewStub(
        grpc::CreateChannel(attention_broker_address, grpc::InsecureChannelCredentials()));
    status = stub->ping(&context, empty, &ack);
    if (status.ok()) {
        LOG_INFO("Connected to AttentionBroker at " << attention_broker_address);
    } else {
        Utils::error("Couldn't connect to AttentionBroker at " + attention_broker_address);
    }
    if (ack.msg() != "PING") {
        Utils::error("Invalid AttentionBroker answer for PING");
    }
}

void RedisMongoDB::redis_setup() {
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
        Utils::warning("Error reading MongoDB chunk size, using default value");
    }
    if (host == "" || port == "" || user == "" || password == "") {
        Utils::error(
            string("You need to set MongoDB access info as environment variables: ") +
            "DAS_MONGODB_HOSTNAME, DAS_MONGODB_PORT, DAS_MONGODB_USERNAME and DAS_MONGODB_PASSWORD");
    }
    string address = host + ":" + port;
    string url = "mongodb://" + user + ":" + password + "@" + address;

    try {
        mongocxx::instance instance;
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

shared_ptr<atomdb_api_types::HandleSet> RedisMongoDB::query_for_pattern(
    const LinkTemplateInterface& link_template) {
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_pattern(link_template);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    unsigned int redis_cursor = 0;
    bool redis_has_more = true;
    string command;
    redisReply* reply;

    auto ctx = this->redis_pool->acquire();

    auto pattern_handle = link_template.get_handle();
    auto handle_set = make_shared<atomdb_api_types::HandleSetRedis>();

    while (redis_has_more) {
        command = ("ZRANGE " + REDIS_PATTERNS_PREFIX + ":" + pattern_handle + " " +
                   to_string(redis_cursor) + " " + to_string(redis_cursor + REDIS_CHUNK_SIZE - 1));

        reply = ctx->execute(command.c_str());

        if (reply == NULL) {
            Utils::error("Redis error");
        }

        if (reply->type != REDIS_REPLY_SET && reply->type != REDIS_REPLY_ARRAY) {
            auto error_type = std::to_string(reply->type);
            freeReplyObject(reply);
            Utils::error("Invalid Redis response: " + error_type);
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

shared_ptr<atomdb_api_types::HandleList> RedisMongoDB::query_for_targets(shared_ptr<char> link_handle) {
    return query_for_targets(link_handle.get());
}

shared_ptr<atomdb_api_types::HandleList> RedisMongoDB::query_for_targets(char* link_handle_ptr) {
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_targets(link_handle_ptr);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    redisReply* reply;
    try {
        auto ctx = this->redis_pool->acquire();
        auto command = "GET " + REDIS_TARGETS_PREFIX + ":" + link_handle_ptr;
        reply = ctx->execute(command.c_str());

        if (reply == NULL) {
            Utils::error("Redis error");
        }

        if ((reply == NULL) || (reply->type == REDIS_REPLY_NIL)) {
            if (this->atomdb_cache != nullptr)
                this->atomdb_cache->add_handle_list(link_handle_ptr, nullptr);
            return nullptr;
        }

        if (reply->type != REDIS_REPLY_STRING) {
            Utils::error("Invalid Redis response: " + std::to_string(reply->type) +
                         " != " + std::to_string(REDIS_REPLY_STRING));
        }

    } catch (const std::exception& e) {
        Utils::error(e.what());
    }

    // NOTE: Intentionally, we aren't destroying 'reply' objects.'reply' objects are destroyed in
    // ~RedisSet().
    auto handle_list = make_shared<atomdb_api_types::RedisStringBundle>(reply);

    if (this->atomdb_cache != nullptr) this->atomdb_cache->add_handle_list(link_handle_ptr, handle_list);
    return handle_list;
}

shared_ptr<atomdb_api_types::AtomDocument> RedisMongoDB::get_atom_document(const char* handle) {
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->get_atom_document(handle);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_COLLECTION_NAME];
    auto reply = mongodb_collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], handle)));

    auto atom_document = reply != bsoncxx::v_noabi::stdx::nullopt
                             ? make_shared<atomdb_api_types::MongodbDocument>(reply)
                             : nullptr;
    if (this->atomdb_cache != nullptr) this->atomdb_cache->add_atom_document(handle, atom_document);
    return atom_document;
}

bool RedisMongoDB::link_exists(const char* link_handle) {
    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_COLLECTION_NAME];
    auto reply = mongodb_collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], link_handle)));
    return (reply != bsoncxx::v_noabi::stdx::nullopt &&
            reply->view().find(MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS]) != reply->view().end());
}

vector<string> RedisMongoDB::links_exist(const vector<string>& link_handles) {
    if (link_handles.empty()) return {};

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_COLLECTION_NAME];

    bsoncxx::builder::basic::array handle_ids;
    for (const auto& handle : link_handles) {
        handle_ids.append(handle);
    }

    bsoncxx::builder::basic::document filter_builder;
    filter_builder.append(bsoncxx::builder::basic::kvp(
        MONGODB_FIELD_NAME[MONGODB_FIELD::ID],
        bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("$in", handle_ids))));

    auto filter = filter_builder.extract();

    auto cursor = mongodb_collection.distinct(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], filter.view());

    vector<string> existing_links;
    for (const auto& doc : cursor) {
        auto values = doc["values"].get_array().value;
        for (auto&& val : values) {
            existing_links.push_back(val.get_string().value.data());
        }
    }

    return existing_links;
}

char* RedisMongoDB::add_node(const atomspace::Node* node) {
    // TODO: Implement add_node logic
    return NULL;
}

char* RedisMongoDB::add_link(const atomspace::Link* link) {
    // TODO: Implement add_link logic
    return NULL;
}

vector<shared_ptr<atomdb_api_types::AtomDocument>> RedisMongoDB::get_atom_documents(
    const vector<string>& handles, const vector<string>& fields) {
    // TODO Add cache support for this method
    vector<shared_ptr<atomdb_api_types::AtomDocument>> atom_documents;

    if (handles.empty()) {
        return atom_documents;
    }

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_COLLECTION_NAME];

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
