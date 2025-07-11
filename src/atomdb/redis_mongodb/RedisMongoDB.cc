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
uint RedisMongoDB::REDIS_CHUNK_SIZE;
string RedisMongoDB::MONGODB_DB_NAME;
string RedisMongoDB::MONGODB_NODES_COLLECTION_NAME;
string RedisMongoDB::MONGODB_LINKS_COLLECTION_NAME;
string RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::size];
uint RedisMongoDB::MONGODB_CHUNK_SIZE;
once_flag RedisMongoDB::MONGODB_INIT_FLAG;

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
        // mongocxx::instance instance;
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

        if (reply == NULL) {
            Utils::error("Redis error");
        }

        if ((reply == NULL) || (reply->type == REDIS_REPLY_NIL)) {
            if (this->atomdb_cache != nullptr) this->atomdb_cache->add_handle_list(handle, nullptr);
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

    if (this->atomdb_cache != nullptr) this->atomdb_cache->add_handle_list(handle, handle_list);
    return handle_list;
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
    if (this->atomdb_cache != nullptr) this->atomdb_cache->add_atom_document(handle, atom_document);
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
    if (documents.size() == handles.size()) {
        return documents;
    }
    auto link_documents = get_link_documents(handles, fields);
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
    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_LINKS_COLLECTION_NAME];

    auto existing_targets_count =
        get_atom_documents(link->targets, {MONGODB_FIELD_NAME[MONGODB_FIELD::ID]}).size();

    cout << "existing_targets_count: " << to_string(existing_targets_count) << endl;
    cout << "link->targets.size(): " << to_string(link->targets.size()) << endl;

    if (existing_targets_count != link->targets.size()) {
        Utils::error("Failed to insert link: " + link->handle() + " has " +
                     to_string(link->targets.size() - existing_targets_count) + " missing targets");
        return "";
    }

    auto mongodb_doc = atomdb_api_types::MongodbDocument(link, *this);
    auto reply = mongodb_collection.insert_one(mongodb_doc.value());

    if (!reply) {
        Utils::error("Failed to insert link into MongoDB");
    }

    return link->handle();
}

vector<string> RedisMongoDB::add_atoms(const vector<atoms::Atom*>& atoms) {
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
    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_NODES_COLLECTION_NAME];

    vector<bsoncxx::v_noabi::document::value> docs;
    vector<string> handles;
    for (const auto& node : nodes) {
        auto mongodb_doc = atomdb_api_types::MongodbDocument(node);
        handles.push_back(node->handle());
        docs.push_back(mongodb_doc.value());
    }

    auto reply = mongodb_collection.insert_many(docs);

    if (!reply) {
        Utils::error("Failed to insert nodes into MongoDB");
    }

    return handles;
}

vector<string> RedisMongoDB::add_links(const vector<atoms::Link*>& links) {
    vector<Link*> links_to_insert;
    for (const auto& link : links) {
        if (get_atom_documents(link->targets, {MONGODB_FIELD_NAME[MONGODB_FIELD::ID]}).size() !=
            link->targets.size()) {
            continue;
        }
        links_to_insert.push_back(link);
    }

    if (links_to_insert.empty()) {
        return {};
    }

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_LINKS_COLLECTION_NAME];

    vector<bsoncxx::v_noabi::document::value> docs;
    vector<string> handles;
    for (const auto& link : links_to_insert) {
        auto mongodb_doc = atomdb_api_types::MongodbDocument(link, *this);
        handles.push_back(link->handle());
        docs.push_back(mongodb_doc.value());
    }

    auto reply = mongodb_collection.insert_many(docs);

    if (!reply) {
        Utils::error("Failed to insert links into MongoDB");
    }

    return handles;
}

bool RedisMongoDB::delete_document(const string& handle, const string& collection_name) {
    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][collection_name];
    auto reply = mongodb_collection.delete_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], handle)));
    return reply->deleted_count() > 0;
}

bool RedisMongoDB::delete_atom(const string& handle) {
    return delete_document(handle, MONGODB_NODES_COLLECTION_NAME) ||
           delete_document(handle, MONGODB_LINKS_COLLECTION_NAME);
}

bool RedisMongoDB::delete_node(const string& handle) {
    return delete_document(handle, MONGODB_NODES_COLLECTION_NAME);
}

bool RedisMongoDB::delete_link(const string& handle) {
    return delete_document(handle, MONGODB_LINKS_COLLECTION_NAME);
}

uint RedisMongoDB::delete_documents(const vector<string>& handles, const string& collection_name) {
    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][collection_name];

    bsoncxx::builder::basic::array handle_ids;
    for (const auto& handle : handles) {
        handle_ids.append(handle);
    }

    bsoncxx::builder::basic::document filter_builder;
    filter_builder.append(bsoncxx::builder::basic::kvp(
        MONGODB_FIELD_NAME[MONGODB_FIELD::ID],
        bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("$in", handle_ids))));

    auto filter = filter_builder.extract();
    auto reply = mongodb_collection.delete_many(filter.view());
    return reply->deleted_count();
}

uint RedisMongoDB::delete_atoms(const vector<string>& handles) {
    auto nodes = delete_nodes(handles);
    auto links = delete_links(handles);
    return nodes + links;
}

uint RedisMongoDB::delete_nodes(const vector<string>& handles) {
    return delete_documents(handles, MONGODB_NODES_COLLECTION_NAME);
}

uint RedisMongoDB::delete_links(const vector<string>& handles) {
    return delete_documents(handles, MONGODB_LINKS_COLLECTION_NAME);
}
