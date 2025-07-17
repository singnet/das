#include "RedisMongoDB.h"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "AttentionBrokerServer.h"
#include "Hasher.h"
#include "Link.h"
#include "Logger.h"
#include "Node.h"
#include "Utils.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"

using namespace atomdb;
using namespace commons;
using namespace atoms;

string RedisMongoDB::REDIS_PATTERNS_PREFIX;
string RedisMongoDB::REDIS_TARGETS_PREFIX;
string RedisMongoDB::REDIS_INCOMING_PREFIX;
uint RedisMongoDB::REDIS_CHUNK_SIZE;
string RedisMongoDB::MONGODB_DB_NAME;
string RedisMongoDB::MONGODB_NODES_COLLECTION_NAME;
string RedisMongoDB::MONGODB_LINKS_COLLECTION_NAME;
string RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::size];
uint RedisMongoDB::MONGODB_CHUNK_SIZE;

RedisMongoDB::RedisMongoDB() {
    redis_setup();
    mongodb_setup();
    attention_broker_setup();
    bool disable_cache = (Utils::get_environment("DAS_DISABLE_ATOMDB_CACHE") == "true");
    this->atomdb_cache = disable_cache ? nullptr : AtomDBCacheSingleton::get_instance();
    this->patterns_next_score.store(get_next_score(REDIS_PATTERNS_PREFIX + ":next_score"));
    this->incoming_set_next_score.store(get_next_score(REDIS_INCOMING_PREFIX + ":next_score"));
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

shared_ptr<Atom> RedisMongoDB::get_atom(const string& handle) {
    shared_ptr<atomdb_api_types::AtomDocument> atom_document = get_atom_document(handle);
    if (atom_document != NULL) {
        if (atom_document->contains(MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS])) {
            unsigned int arity = atom_document->get_size(MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS]);
            vector<string> targets;
            for (unsigned int i = 0; i < arity; i++) {
                targets.push_back(
                    string(atom_document->get(MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS], i)));
            }
            // NOTE TO REVIEWER
            //     TODO We're missing custom_attributes here. I'm not sure how to deal with them.
            //     I guess we should iterate through all the fields in atom_document and add anything
            //     that's not an expected field (named_type, targets, composite_type,
            //     composite_type_hash, etc) as a custom attribute. If this approach is OK, we should add
            //     methods in AtomDocument's API to allow iterate through all the keys, which we
            //     currently don't have.
            return make_shared<Link>(atom_document->get("named_type"), targets);
        } else {
            return make_shared<Node>(atom_document->get("named_type"), atom_document->get("name"));
        }
    } else {
        return shared_ptr<Atom>(NULL);
    }
}

shared_ptr<atomdb_api_types::HandleSet> RedisMongoDB::query_for_pattern(const LinkSchema& link_schema) {
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_pattern(link_schema);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    unsigned int redis_cursor = 0;
    bool redis_has_more = true;
    string command;
    redisReply* reply;

    auto ctx = this->redis_pool->acquire();

    auto pattern_handle = link_schema.handle();
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
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_targets(handle);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    redisReply* reply;
    try {
        auto ctx = this->redis_pool->acquire();
        auto command = "GET " + REDIS_TARGETS_PREFIX + ":" + handle;
        reply = ctx->execute(command.c_str());

        if (reply == NULL) Utils::error("Redis error at query_for_targets");

        if ((reply == NULL) || (reply->type == REDIS_REPLY_NIL)) {
            if (this->atomdb_cache != nullptr) this->atomdb_cache->add_handle_list(handle, nullptr);
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

    if (this->atomdb_cache != nullptr) this->atomdb_cache->add_handle_list(handle, handle_list);
    return handle_list;
}

shared_ptr<atomdb_api_types::HandleSet> RedisMongoDB::query_for_incoming_set(const string& handle) {
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

void RedisMongoDB::delete_pattern(const string& handle) {
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
    auto ctx = this->redis_pool->acquire();
    string command = "ZREM " + REDIS_PATTERNS_PREFIX + ":" + key + " " + value;
    redisReply* reply = ctx->execute(command.c_str());
    if (reply == NULL) Utils::error("Redis error at update_pattern");

    if (this->atomdb_cache != nullptr) this->atomdb_cache->erase_pattern_matching_cache(key);
}

uint RedisMongoDB::get_next_score(const string& key) {
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
    auto ctx = this->redis_pool->acquire();
    string command = "SET " + key + " " + to_string(score);
    redisReply* reply = ctx->execute(command.c_str());
    if (reply == NULL) Utils::error("Redis error at set_next_score");

    if (reply->type != REDIS_REPLY_STATUS) {
        Utils::error("Invalid Redis response at set_next_score: " + std::to_string(reply->type));
    }
}

void RedisMongoDB::add_incoming_set(const string& handle, const string& incoming_handle) {
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
        auto cache_result = this->atomdb_cache->get_atom_document(handle);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][collection_name];
    auto reply = mongodb_collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], handle)));

    auto atom_document = reply != bsoncxx::v_noabi::stdx::nullopt
                             ? make_shared<atomdb_api_types::MongodbDocument>(reply)
                             : nullptr;
    if (this->atomdb_cache != nullptr && atom_document != nullptr)
        this->atomdb_cache->add_atom_document(handle, atom_document);
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

    // TODO(arturgontijo): Fetch LTs from MongoDB to update patterns.

    for (const auto& target : existing_targets) {
        add_incoming_set(target->get(MONGODB_FIELD_NAME[MONGODB_FIELD::ID]), link->handle());
    }

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

// TODO(arturgontijo): Need to delete handle from patterns set.
bool RedisMongoDB::delete_document(const string& handle,
                                   const string& collection_name,
                                   bool delete_targets) {
    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][collection_name];
    auto reply = mongodb_collection.delete_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], handle)));

    auto incoming_set = query_for_incoming_set(handle);
    auto it = incoming_set->get_iterator();
    char* incoming_handle;
    while ((incoming_handle = it->next()) != nullptr) {
        delete_atom(incoming_handle, delete_targets);
    }
    delete_incoming_set(handle);

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
        if (query_for_incoming_set(target_handle)->size() > 1) {
            update_incoming_set(target_handle, handle);
        } else if (delete_targets) {
            delete_atom(target_handle, delete_targets);
        }
        targets.push_back(target_handle);
    }

    // TODO(arturgontijo): Fetch LTs from MongoDB to update patterns.

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
