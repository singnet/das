#include "MorkMongoDB.h"

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

string MorkMongoDB::MONGODB_DB_NAME;
string MorkMongoDB::MONGODB_COLLECTION_NAME;
string MorkMongoDB::MONGODB_FIELD__NAME[MONGODB_FIELD_::size_];
uint MorkMongoDB::MONGODB_CHUNK_SIZE;

// --> MorkClient
MorkClient::MorkClient(const string& base_url)
    : base_url_(Utils::trim(base_url.empty() ? Utils::get_environment("MORK_URL") : base_url)),
    cli(httplib::Client(base_url)) {
    send_request("GET", "/status/-");
}
MorkClient::~MorkClient() {}
string MorkClient::post(
    const string& data, const string& pattern, const string& template_) {
    auto path = "/upload/" + url_encode(pattern) + "/" + url_encode(template_);
    auto res = send_request("POST", path, data);
    return res->body;
}
vector<string> MorkClient::get(const string& pattern, const string& template_) {
    auto path = "/export/" + url_encode(pattern) + "/" + url_encode(template_);
    auto res = send_request("GET", path);
    return Utils::split(res->body, '\n');
}
httplib::Result MorkClient::send_request(
    const string& method, const string& path, const string& data) {
    httplib::Result res;

    // TODO: Improve the way build with schema
    string url = "http://" + this->base_url_ + path;
    
    if (method == "GET") {
        res = this->cli.Get(url);
    } else if (method == "POST") {
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
string MorkClient::url_encode(const string& value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex << uppercase;

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

// --> MorkMongoDB
MorkMongoDB::MorkMongoDB() {
    mork_setup();
    mongodb_setup();
    attention_broker_setup();
    bool disable_cache = (Utils::get_environment("DAS_DISABLE_ATOMDB_CACHE") == "true");
    this->atomdb_cache = disable_cache ? nullptr : AtomDBCacheSingleton::get_instance();
}
MorkMongoDB::~MorkMongoDB() {
    delete this->mongodb_pool;
}
void MorkMongoDB::mork_setup() {
    string host = Utils::get_environment("DAS_MORK_HOSTNAME");
    string port = Utils::get_environment("DAS_MORK_PORT");
    string address = host + ":" + port;

    if (host == "" || port == "") {
        Utils::error(
            "You need to set MORK access info as environment variables: DAS_MORK_HOSTNAME, "
            "DAS_MORK_PORT");
    }

    try {
        this->mork_client = make_shared<MorkClient>(address);
        LOG_INFO("Connected to MORK at " << address);
    } catch (const exception& e) {
        Utils::error(e.what());
    }
}
void MorkMongoDB::mongodb_setup() {
    string host = Utils::get_environment("DAS_MONGODB_HOSTNAME");
    string port = Utils::get_environment("DAS_MONGODB_PORT");
    string user = Utils::get_environment("DAS_MONGODB_USERNAME");
    string password = Utils::get_environment("DAS_MONGODB_PASSWORD");
    try {
        uint chunk_size = Utils::string_to_int(Utils::get_environment("DAS_MONGODB_CHUNK_SIZE"));
        if (chunk_size > 0) {
            MONGODB_CHUNK_SIZE = chunk_size;
        }
    } catch (const exception& e) {
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
    } catch (const exception& e) {
        Utils::error(e.what());
    }
}
void MorkMongoDB::attention_broker_setup() {
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
        cout << "Connected to AttentionBroker at " << attention_broker_address << endl;
    } else {
        Utils::error("Couldn't connect to AttentionBroker at " + attention_broker_address);
    }
    if (ack.msg() != "PING") {
        Utils::error("Invalid AttentionBroker answer for PING");
    }
}
shared_ptr<atomdb_api_types::HandleSet> MorkMongoDB::query_for_pattern(
    const LinkTemplateInterface& link_template)
{
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_pattern(link_template);
        if (cache_result.is_cache_hit) return cache_result.result;
    }
    // WIP - This method will be implemented in the LinkTemplate 
    string pattern_metta = link_template.get_metta_expression();
    // template should equals to pattern_metta
    vector<string> raw_expressions = this->mork_client->get(pattern_metta, pattern_metta);

    auto handle_set = make_shared<atomdb_api_types::HandleSetMork>();

    for (const auto& raw_expr : raw_expressions) {
        Node root_node = parse_expression_tree(raw_expr);
        auto handle = resolve_node_handle(root_node);
        handle_set->append(make_shared<atomdb_api_types::HandleSetMork>(handle));
    }

    if (this->atomdb_cache != nullptr)
        this->atomdb_cache->add_pattern_matching(link_template.get_handle(), handle_set);

    return handle_set;
}
vector<string> MorkMongoDB::tokenize_expression(const string& expr) {
    vector<string> tokens;
    regex re(R"(\(|\)|"[^"]*"|[^\s()]+)");
    auto begin = sregex_iterator(expr.begin(), expr.end(), re);
    auto end   = sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        tokens.push_back(it->str());
    }
    return tokens;
}
MorkMongoDB::Node MorkMongoDB::parse_tokens_to_node(const vector<string>& tokens, size_t& pos) {
    Node node;
    if (tokens[pos] == "(") {
        ++pos;
        while (pos < tokens.size() && tokens[pos] != ")") {
            if (tokens[pos] == "(") {
                node.targets.push_back(parse_tokens_to_node(tokens, pos));
            } else {
                // Node leaf;
                // leaf.name = tokens[pos++];
                Node leaf{ tokens[pos++], {} };
                node.targets.push_back(move(leaf));
            }
        }
        if (pos < tokens.size() && tokens[pos] == ")") {
           ++pos;
        }
    } else {
        node.name = tokens[pos++];
    }
    return node;
}
MorkMongoDB::Node MorkMongoDB::parse_expression_tree(const string& expr) {
    auto tokens = tokenize_expression(expr);
    size_t pos = 0;
    return parse_tokens_to_node(tokens, pos);
}
string MorkMongoDB::resolve_node_handle(Node& node) {
    auto conn = this->mongodb_pool->acquire();
    auto collection = (*conn)[MONGODB_DB_NAME][MONGODB_COLLECTION_NAME];

    bsoncxx::builder::basic::document filter_builder;
    mongocxx::options::find find_opts;

    if (node.targets.empty()) {
        filter_builder.append(
            bsoncxx::builder::basic::kvp(
                MONGODB_FIELD__NAME[MONGODB_FIELD_::NAME_], node.name));
    } else {
        bsoncxx::builder::basic::array children_array;
        for (auto& child : node.targets) {
            children_array.append(resolve_node_handle(child));
        }

        filter_builder.append(
            bsoncxx::builder::basic::kvp(
                MONGODB_FIELD__NAME[MONGODB_FIELD_::TARGETS_],
                bsoncxx::builder::basic::make_document(
                    bsoncxx::builder::basic::kvp("$all", children_array.view()))));
        
        bsoncxx::builder::basic::document proj_builder;
        proj_builder.append(
            bsoncxx::builder::basic::kvp(MONGODB_FIELD__NAME[MONGODB_FIELD_::ID_], 1));
        find_opts.projection(proj_builder.view());
    }
    
    auto result = collection.find_one(filter_builder.view(), find_opts);
    if (!result) return "";
    
    auto id_element = (*result)[MONGODB_FIELD__NAME[MONGODB_FIELD_::ID_]];
    return id_element.get_string().value.data();
}
shared_ptr<atomdb_api_types::HandleList> MorkMongoDB::query_for_targets(shared_ptr<char> link_handle) {
    return query_for_targets(link_handle.get());
}
// WIP
shared_ptr<atomdb_api_types::HandleList> MorkMongoDB::query_for_targets(char* link_handle_ptr) {
    return NULL;
}
shared_ptr<atomdb_api_types::AtomDocument> MorkMongoDB::get_atom_document(const char* handle) {
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->get_atom_document(handle);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_COLLECTION_NAME];
    auto reply = mongodb_collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD__NAME[MONGODB_FIELD_::ID_], handle)));
    auto atom_document = reply != bsoncxx::v_noabi::stdx::nullopt ? make_shared<atomdb_api_types::MongodbDocument2>(reply) : nullptr;
    if (this->atomdb_cache != nullptr) this->atomdb_cache->add_atom_document(handle, atom_document);
    return atom_document;
}
bool MorkMongoDB::link_exists(const char* link_handle) {
    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_COLLECTION_NAME];
    auto reply = mongodb_collection.find_one(bsoncxx::v_noabi::builder::basic::make_document(
        bsoncxx::v_noabi::builder::basic::kvp(MONGODB_FIELD__NAME[MONGODB_FIELD_::ID_], link_handle)));
    return (reply != bsoncxx::v_noabi::stdx::nullopt &&
            reply->view().find(MONGODB_FIELD__NAME[MONGODB_FIELD_::TARGETS_]) != reply->view().end());
}
vector<string> MorkMongoDB::links_exist(const vector<string>& link_handles) {
    if (link_handles.empty()) return {};

    auto conn = this->mongodb_pool->acquire();
    auto mongodb_collection = (*conn)[MONGODB_DB_NAME][MONGODB_COLLECTION_NAME];

    bsoncxx::builder::basic::array handle_ids;
    for (const auto& handle : link_handles) {
        handle_ids.append(handle);
    }

    bsoncxx::builder::basic::document filter_builder;
    filter_builder.append(bsoncxx::builder::basic::kvp(
        MONGODB_FIELD__NAME[MONGODB_FIELD_::ID_],
        bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("$in", handle_ids))));

    auto filter = filter_builder.extract();

    auto cursor = mongodb_collection.distinct(MONGODB_FIELD__NAME[MONGODB_FIELD_::ID_], filter.view());

    vector<string> existing_links;
    for (const auto& doc : cursor) {
        auto values = doc["values"].get_array().value;
        for (auto&& val : values) {
            existing_links.push_back(val.get_string().value.data());
        }
    }

    return existing_links;
}
vector<shared_ptr<atomdb_api_types::AtomDocument>> MorkMongoDB::get_atom_documents(
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
            auto array = filter_builder << MONGODB_FIELD__NAME[MONGODB_FIELD_::ID_]
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
                auto atom_document = make_shared<atomdb_api_types::MongodbDocument2>(opt_val);
                atom_documents.push_back(atom_document);
            }
        }
    } catch (const exception& e) {
        Utils::error("MongoDB error: " + string(e.what()));
    }

    return atom_documents;
}
char* MorkMongoDB::add_node(const atomspace::Node* node) { return NULL; }
char* MorkMongoDB::add_link(const atomspace::Link* link) { return NULL; }
// <--