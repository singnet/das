#include "MorkMongoDB.h"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "AttentionBrokerServer.h"
#include "LinkTemplate.h"
#include "Utils.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"

using namespace atomdb;
using namespace commons;

std::string MorkMongoDB::REDIS_PATTERNS_PREFIX;
std::string MorkMongoDB::REDIS_TARGETS_PREFIX;
uint MorkMongoDB::REDIS_CHUNK_SIZE;
std::string MorkMongoDB::MONGODB_DB_NAME;
std::string MorkMongoDB::MONGODB_COLLECTION_NAME;
std::string MorkMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::size];

// --> MorkClient
MorkClient::MorkClient(const std::string& base_url)
    : base_url_(Utils::trim(base_url.empty() ? Utils::get_environment("MORK_URL") : base_url)),
      cli(httplib::Client(base_url)) {
    sendRequest("GET", "/status/-");
}
MorkClient::~MorkClient() {}
// unstable
std::string MorkClient::post(const std::string& data,
                             const std::string& pattern,
                             const std::string& template_) {
    auto path = "/upload/" + urlEncode(pattern) + "/" + urlEncode(template_);
    auto res = sendRequest("POST", path, data);
    return res->body;
}
std::vector<std::string> MorkClient::get(const std::string& pattern, const std::string& template_) {
    auto path = "/export/" + urlEncode(pattern) + "/" + urlEncode(template_);
    auto res sendRequest("GET", path) return Utils::split(res->body, "\n");
}
httplib::Result MorkClient::sendRequest(const std::string& method,
                                        const std::string& path,
                                        const std::string& data) {
    httplib::Result res;
    std::string url = this->base_url_ + path;

    if (method == 'GET') {
        res = this->cli.Get(url);
    } else if (method == 'POST') {
        if (data.empty()) {
            Utils::error("POST request data is empty. Cannot send an empty payload to MORK server.");
        }
        res = this->cli.Post(url, data, "text/plain");
    }

    if (!res) {
        Utils::error("Connection error at " + url);
    } else if (res->status != httplib::StatusCode::OK_200) {
        auto err = res.error();
        Utils::error("Http error: " + httplib::to_string(err));
    }

    return res;
}
std::string MorkClient::urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex << std::uppercase;

    for (const char c : value) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }

    return escaped.str();
}
// <--

MorkMongoDB::MorkMongoDB() {
    mork_setup();
    mongodb_setup();
    attention_broker_setup();
    bool disable_cache = (Utils::get_environment("DAS_DISABLE_ATOMDB_CACHE") == "true");
    this->atomdb_cache = disable_cache ? nullptr : AtomDBCacheSingleton::get_instance();
}
MorkMongoDB::~MorkMongoDB() {
    delete this->mongodb_pool;
    // delete this->mongodb_client;
}
void MorkMongoDB::mork_setup() {
    std::string host = Utils::get_environment("DAS_MORK_HOSTNAME");
    std::string port = Utils::get_environment("DAS_MORK_PORT");
    std::string address = host + ":" + port;

    if (host == "" || port == "") {
        Utils::error(
            "You need to set MORK access info as environment variables: DAS_MORK_HOSTNAME, "
            "DAS_MORK_PORT");
    }

    try {
        this->mork_client = MorkClient(address);
        cout << "Connected to MORK at " << address << endl;
    } catch (const std::exception& e) {
        Utils::error(e.what());
    }
}
void MorkMongoDB::mongodb_setup() {
    std::string host = Utils::get_environment("DAS_MONGODB_HOSTNAME");
    std::string port = Utils::get_environment("DAS_MONGODB_PORT");
    std::string user = Utils::get_environment("DAS_MONGODB_USERNAME");
    std::string password = Utils::get_environment("DAS_MONGODB_PASSWORD");
    if (host == "" || port == "" || user == "" || password == "") {
        Utils::error(
            std::string("You need to set MongoDB access info as environment variables: ") +
            "DAS_MONGODB_HOSTNAME, DAS_MONGODB_PORT, DAS_MONGODB_USERNAME and DAS_MONGODB_PASSWORD");
    }
    std::string address = host + ":" + port;
    std::string url = "mongodb://" + user + ":" + password + "@" + address;

    try {
        mongocxx::instance instance;
        auto uri = mongocxx::uri{url};
        this->mongodb_pool = new mongocxx::pool(uri);
        this->mongodb = get_database();
        const auto ping_cmd =
            bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1));
        this->mongodb.run_command(ping_cmd.view());
        this->mongodb_collection = this->mongodb[MONGODB_COLLECTION_NAME];
        std::cout << "Connected to MongoDB at " << address << endl;
    } catch (const std::exception& e) {
        Utils::error(e.what());
    }
}
void MorkMongoDB::attention_broker_setup() {
    grpc::ClientContext context;
    grpc::Status status;
    dasproto::Empty empty;
    dasproto::Ack ack;
    std::string attention_broker_address = Utils::get_environment("DAS_ATTENTION_BROKER_ADDRESS");
    std::string attention_broker_port = Utils::get_environment("DAS_ATTENTION_BROKER_PORT");

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
mongocxx::database MorkMongoDB::get_database() {
    auto database = this->mongodb_pool->acquire();
    return database[MONGODB_DB_NAME];
}
std::shared_ptr<atomdb_api_types::HandleSet> MorkMongoDB::query_for_pattern(
    const LinkTemplateInterface& link_template)
{
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_pattern(link_template);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    std::string pattern_metta = link_template.get_metta_expr();
    
    // template should equals to pattern_metta
    std::vector<std::string> response = this->mork_client->get(pattern_metta, pattern_metta);

    auto handle_set = make_shared<atomdb_api_types::HandleSetMork>();

    for (size_t i = 0; < response.size(); ++i) {
        auto expr = response[i];
        Node atom = parse_expression(expr);
        auto handle = get_handle(atom);
        handle_set->append(make_shared<atomdb_api_types::HandleSetMork>(handle));
    }

    if (this->atomdb_cache != nullptr)
        this->atomdb_cache->add_pattern_matching(pattern_handle.get(), handle_set);

    return handle_set;
}
std::vector<std::string> MorkMongoDB::tokenize(const std::string& expr) {
    std::vector<std::string> tokens;
    std::regex re(R"(\(|\)|"[^"]*"|[^\s()]+)");
    auto begin = std::sregex_iterator(expr.begin(), expr.end(), re);
    auto end   = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        tokens.push_back(it->str());
    }
    return tokens;
}
Node MorkMongoDB::parse_tokens(const std::vector<std::string>& tokens, size_t& pos) {
    Node root_atom;

    if (tokens[pos] == "(") {
        ++pos;

        while (pos < tokens.size() && tokens[pos] != ")") {
            if (tokens[pos] == "(") {
                root_atom.targets.push_back(parse_tokens(tokens, pos));
            } else {
                Node atom_leaf;
                atom_leaf.name = tokens[pos++];
                root_atom.targets.push_back(std::move(atom_leaf));
            }
        }
        if (pos < tokens.size() && tokens[pos] == ")") {
            ++pos;
        }
    } else {
        root_atom.name = tokens[pos++];
    }
    return root_atom;
}
Node MorkMongoDB::parse_expression(const std::string& expr) {
    auto tokens = tokenize(expr);
    return parse_tokens(tokens, 0);
}
std::string MorkMongoDB::get_handle(Node& atom) {
    if (atom.targets.empty()) {
        lock_guard<mutex> lock(this->mongodb_mutex);;
        auto mongodb_collection = get_database()[MONGODB_COLLECTION_NAME];

        bsoncxx::builder::basic::document filter_builder;
        filter_builder.append(bsoncxx::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::NAME], atom.name));
        
        bsoncxx::builder::basic::document projection_builder;
        projection_builder.append(bsoncxx::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], 1));
        mongocxx::options::find options;
        options.projection(projection_builder.view());

        auto reply = mongodb_collection.find_one(filter_builder.view(), options);

        if (reply != core::v1::nullopt) {
            return (*reply)[MONGODB_FIELD_NAME[MONGODB_FIELD::ID]].get_string().value.to_string();
        }
        return "";
        // auto reply = mongodb_collection.find_one(
        //     bsoncxx::v_noabi::builder::basic::make_document(
        //         bsoncxx::v_noabi::builder::basic::kvp(
        //             MONGODB_FIELD_NAME[MONGODB_FIELD::NAME], atom.name)));
        
        // auto atom_document = reply != core::v1::nullopt ? make_shared<atomdb_api_types::MongodbDocument>(reply) : nullptr;
        
        // if (atom_document) {
        //     return atom_document.get(MONGODB_FIELD_NAME[MONGODB_FIELD::NAME]);
        // }
        // return "";
    } else {
        std::vector<std::string> handles;
        for (size_t i = 0; i < atom.targets.size(); ++i) {
            handles.push_back(get_handle(atom.targets[i]));
        }
        
        lock_guard<mutex> lock(this->mongodb_mutex);
        auto mongodb_collection = get_database()[MONGO_COLLECTION_NAME];
        
        bsoncxx::builder::basic::array array_builder;
        for (const auto& handle : handles)
            array_builder.append(handle);
        
        bsoncxx::builder::basic::document filter_builder;
        filter_builder.append(
            bsoncxx::builder::basic::kvp(
                MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS],
                bsoncxx::builder::basic::make_document(
                    bsoncxx::builder::basic::kvp("$all", arr_builder.view())
                )
            )
        );

        bsoncxx::builder::basic::document projection_builder;
        projection_builder.append(bsoncxx::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], 1));
        mongocxx::options::find options;
        options.projection(projection_builder.view());

        auto reply = mongodb_collection.find_one(filter_builder.view(), options);

        if (reply != core::v1::nullopt) {
            return (*reply)[MONGODB_FIELD_NAME[MONGODB_FIELD::ID]].get_string().value.to_string();
        }
        return "";
    }
}
shared_ptr<atomdb_api_types::HandleList> MorkMongoDB::query_for_targets(shared_ptr<char> link_handle) {
    return query_for_targets(link_handle.get());
}
shared_ptr<atomdb_api_types::HandleList> MorkMongoDB::query_for_targets(char* link_handle_ptr) {
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_targets(link_handle_ptr);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    redisReply* reply = (redisReply*) redisCommand(
        this->redis_single, "GET %s:%s", REDIS_TARGETS_PREFIX.c_str(), link_handle_ptr);

    if ((reply == NULL) || (reply->type == REDIS_REPLY_NIL)) {
        if (this->atomdb_cache != nullptr) this->atomdb_cache->add_handle_list(link_handle_ptr, nullptr);
        return nullptr;
    }
    if (reply->type != REDIS_REPLY_STRING) {
        Utils::error("Invalid Redis response: " + std::to_string(reply->type) +
                     " != " + std::to_string(REDIS_REPLY_STRING));
    }

    auto handle_list = make_shared<atomdb_api_types::RedisStringBundle>(reply);
    if (this->atomdb_cache != nullptr) this->atomdb_cache->add_handle_list(link_handle_ptr, handle_list);
    return handle_list;
}
shared_ptr<atomdb_api_types::AtomDocument> MorkMongoDB::get_atom_document(const char* handle) {
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->get_atom_document(handle);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    this->mongodb_mutex.lock();
    auto mongodb_collection = get_database()[MONGODB_COLLECTION_NAME];
    auto reply = mongodb_collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], handle)));
    this->mongodb_mutex.unlock();

    auto atom_document =
        reply != core::v1::nullopt ? make_shared<atomdb_api_types::MongodbDocument>(reply) : nullptr;

    if (this->atomdb_cache != nullptr) this->atomdb_cache->add_atom_document(handle, atom_document);

    return atom_document;
}
bool MorkMongoDB::link_exists(const char* link_handle) {
    this->mongodb_mutex.lock();
    auto mongodb_collection = get_database()[MONGODB_COLLECTION_NAME];
    auto reply = mongodb_collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], link_handle)));
    this->mongodb_mutex.unlock();
    return (reply != core::v1::nullopt &&
            reply->view().find(MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS]) != reply->view().end());
}
std::vector<std::string> MorkMongoDB::links_exist(const std::vector<std::string>& link_handles) {
    if (link_handles.empty()) return {};

    lock_guard<mutex> lock(this->mongodb_mutex);
    auto mongodb_collection = get_database()[MONGODB_COLLECTION_NAME];

    bsoncxx::builder::basic::document filter_builder;
    bsoncxx::builder::basic::array array_builder;

    for (const auto& handle : link_handles) array_builder.append(handle);

    filter_builder.append(
        bsoncxx::builder::basic::kvp(
            MONGODB_FIELD_NAME[MONGODB_FIELD::ID],
            bsoncxx::builder::basic::make_document(
                bsoncxx::builder::basic::kvp("$in", array_builder.view()))));

    // Only project the ID field
    bsoncxx::builder::basic::document projection_builder;
    projection_builder.append(bsoncxx::builder::basic::kvp(MONGODB_FIELD_NAME[MONGODB_FIELD::ID], 1));

    mongocxx::options::find options;
    options.projection(projection_builder.view());

    auto cursor = mongodb_collection.find(filter_builder.view(), options);

    std::vector<string> existing_links;
    for (const auto& doc : cursor) {
        existing_links.push_back(
            doc[MONGODB_FIELD_NAME[MONGODB_FIELD::ID]].get_string().value.to_string());
    }

    return existing_links;
}
char* MorkMongoDB::add_node(const char* type,
                            const char* name,
                            const atomdb_api_types::CustomAttributesMap& custom_attributes) {
    return NULL;
}  // TODO: Implement add_node logic
char* MorkMongoDB::add_link(const char* type,
                            char** targets,
                            size_t targets_size,
                            const atomdb_api_types::CustomAttributesMap& custom_attributes) {
    return NULL;
}  // TODO: Implement add_link logic
