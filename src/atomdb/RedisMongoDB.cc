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

RedisMongoDB::RedisMongoDB() {
    redis_setup();
    mongodb_setup();
    attention_broker_setup();
    bool disable_cache = (Utils::get_environment("DAS_DISABLE_ATOMDB_CACHE") == "true");
    this->atomdb_cache = disable_cache ? nullptr : AtomDBCacheSingleton::get_instance();
}

RedisMongoDB::~RedisMongoDB() {
    if (this->redis_cluster != NULL) {
        redisClusterFree(this->redis_cluster);
    }
    if (this->redis_single != NULL) {
        redisFree(this->redis_single);
    }
    delete this->mongodb_pool;
    // delete this->mongodb_client;
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
        std::cout << "Connected to AttentionBroker at " << attention_broker_address << endl;
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
    string cluster_tag = (this->cluster_flag ? "CLUSTER" : "NON-CLUSTER");

    if (this->cluster_flag) {
        this->redis_cluster = redisClusterConnect(address.c_str(), 0);
        this->redis_single = NULL;
    } else {
        this->redis_single = redisConnect(host.c_str(), stoi(port));
        this->redis_cluster = NULL;
    }

    if (this->redis_cluster == NULL && this->redis_single == NULL) {
        Utils::error("Connection error.");
    } else if ((!this->cluster_flag) && this->redis_single->err) {
        Utils::error("Redis error: " + string(this->redis_single->errstr));
    } else if (this->cluster_flag && this->redis_cluster->err) {
        Utils::error("Redis cluster error: " + string(this->redis_cluster->errstr));
    } else {
        cout << "Connected to (" << cluster_tag << ") Redis at " << address << endl;
    }
}

mongocxx::database RedisMongoDB::get_database() {
    auto database = this->mongodb_pool->acquire();
    return database[MONGODB_DB_NAME];
}

void RedisMongoDB::mongodb_setup() {
    string host = Utils::get_environment("DAS_MONGODB_HOSTNAME");
    string port = Utils::get_environment("DAS_MONGODB_PORT");
    string user = Utils::get_environment("DAS_MONGODB_USERNAME");
    string password = Utils::get_environment("DAS_MONGODB_PASSWORD");
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
        this->mongodb = get_database();

        // this->mongodb_client = new mongocxx::client(uri);
        // this->mongodb = (*this->mongodb_client)[MONGODB_DB_NAME];
        const auto ping_cmd =
            bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1));
        this->mongodb.run_command(ping_cmd.view());
        this->mongodb_collection = this->mongodb[MONGODB_COLLECTION_NAME];
        // auto atom_count = this->mongodb_collection.count_documents({});
        // std::cout << "Connected to MongoDB at " << address << " Atom count: " << atom_count << endl;
        std::cout << "Connected to MongoDB at " << address << endl;
    } catch (const std::exception& e) {
        Utils::error(e.what());
    }
}

shared_ptr<atomdb_api_types::HandleSet> RedisMongoDB::query_for_pattern(
    std::shared_ptr<char> pattern_handle) {
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_pattern(pattern_handle.get());
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    unsigned int redis_cursor = 0;
    bool redis_has_more = true;
    string command;
    redisReply* reply;

    auto handle_set = make_shared<atomdb_api_types::HandleSetRedis>();

    while (redis_has_more) {
        command = ("ZRANGE " + REDIS_PATTERNS_PREFIX + ":" + pattern_handle.get() + " " +
                   to_string(redis_cursor) + " " + to_string(redis_cursor + REDIS_CHUNK_SIZE - 1));

        if (this->cluster_flag) {
            reply = (redisReply*) redisClusterCommand(this->redis_cluster, command.c_str());
        } else {
            reply = (redisReply*) redisCommand(this->redis_single, command.c_str());
        }

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
        this->atomdb_cache->add_pattern_matching(pattern_handle.get(), handle_set);
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
        if (this->cluster_flag) {
            reply = (redisReply*) redisClusterCommand(
                this->redis_cluster, "GET %s:%s", REDIS_TARGETS_PREFIX.c_str(), link_handle_ptr);
        } else {
            reply = (redisReply*) redisCommand(
                this->redis_single, "GET %s:%s", REDIS_TARGETS_PREFIX.c_str(), link_handle_ptr);
        }

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

    this->mongodb_mutex.lock();
    auto mongodb_collection = get_database()[MONGODB_COLLECTION_NAME];
    auto reply = mongodb_collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], handle)));
    // cout << bsoncxx::to_json(*reply) << endl; // Note to reviewer: please let this dead code here
    this->mongodb_mutex.unlock();

    auto atom_document = make_shared<atomdb_api_types::MongodbDocument>(reply);
    if (this->atomdb_cache != nullptr) this->atomdb_cache->add_atom_document(handle, atom_document);
    return atom_document;
}

bool RedisMongoDB::link_exists(const char* link_handle) {
    this->mongodb_mutex.lock();
    auto mongodb_collection = get_database()[MONGODB_COLLECTION_NAME];
    auto reply = mongodb_collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], link_handle)));
    this->mongodb_mutex.unlock();
    return reply != core::v1::nullopt;
}

vector<string> RedisMongoDB::links_exist(const vector<string>& link_handles) {
    if (link_handles.empty()) return {};

    lock_guard<mutex> lock(this->mongodb_mutex);
    auto mongodb_collection = get_database()[MONGODB_COLLECTION_NAME];

    bsoncxx::builder::basic::document filter_builder;
    bsoncxx::builder::basic::array array_builder;

    for (const auto& handle : link_handles) array_builder.append(handle);

    filter_builder.append(
        bsoncxx::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID],
                                     bsoncxx::builder::basic::make_document(
                                         bsoncxx::builder::basic::kvp("$in", array_builder.view()))));

    // Only project the ID field
    bsoncxx::builder::basic::document projection_builder;
    projection_builder.append(bsoncxx::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], 1));

    mongocxx::options::find options;
    options.projection(projection_builder.view());

    auto cursor = mongodb_collection.find(filter_builder.view(), options);

    vector<string> existing_links;
    for (const auto& doc : cursor) {
        existing_links.push_back(
            doc[MONGODB_FIELD_NAME[MONGODB_FIELD::ID]].get_string().value.to_string());
    }

    return existing_links;
}

vector<shared_ptr<atomdb_api_types::AtomDocument>> RedisMongoDB::get_atom_documents(
    vector<string>& handles, vector<string>& fields) {
    // TODO Add cache support for this method
    this->mongodb_mutex.lock();
    vector<shared_ptr<atomdb_api_types::AtomDocument>> atom_documents;
    auto mongodb_collection = get_database()[MONGODB_COLLECTION_NAME];
    bsoncxx::builder::stream::document filter_builder;
    auto array = filter_builder << MONGODB_FIELD_NAME[MONGODB_FIELD::ID]
                                << bsoncxx::builder::stream::open_document << "$in"
                                << bsoncxx::builder::stream::open_array;
    for (const auto& id : handles) {
        array << id;
    }
    array << bsoncxx::builder::stream::close_array << bsoncxx::builder::stream::close_document;
    auto filter = filter_builder.view();
    bsoncxx::builder::stream::document projection_builder;
    for (const auto& field : fields) {
        projection_builder << field << 1;  // Include the field in the projection
    }
    auto cursor =
        mongodb_collection.find(filter, mongocxx::options::find{}.projection(projection_builder.view()));
    for (const auto& view : cursor) {
        core::v1::optional<bsoncxx::v_noabi::document::value> opt_val =
            bsoncxx::v_noabi::document::value(view);
        auto atom_document = make_shared<atomdb_api_types::MongodbDocument>(opt_val);
        atom_documents.push_back(atom_document);
    }
    this->mongodb_mutex.unlock();
    return atom_documents;
}
