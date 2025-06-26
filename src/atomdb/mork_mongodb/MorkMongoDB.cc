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

// --> MorkClient
MorkClient::MorkClient(const string& base_url)
    : base_url_(Utils::trim(base_url.empty() ? Utils::get_environment("MORK_URL") : base_url)),
      cli(httplib::Client(base_url)) {
    send_request("GET", "/status/-");
}
MorkClient::~MorkClient() {}
string MorkClient::post(const string& data, const string& pattern, const string& template_) {
    auto path = "/upload/" + url_encode(pattern) + "/" + url_encode(template_);
    auto res = send_request("POST", path, data);
    return res->body;
}
vector<string> MorkClient::get(const string& pattern, const string& template_) {
    auto path = "/export/" + url_encode(pattern) + "/" + url_encode(template_);
    auto res = send_request("GET", path);
    return Utils::split(res->body, '\n');
}
httplib::Result MorkClient::send_request(const string& method, const string& path, const string& data) {
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
    this->redis_mongodb = make_shared<RedisMongoDB>();
    this->mongodb_pool = this->redis_mongodb->get_mongo_pool();
    this->atomdb_cache = this->redis_mongodb->get_atomdb_cache();
    mork_setup();
}
MorkMongoDB::~MorkMongoDB() { delete this->mongodb_pool; }

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
    const LinkTemplateInterface& link_template) {
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
    auto end = sregex_iterator();
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
                Node leaf{tokens[pos++], {}};
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
    auto collection = (*conn)[RedisMongoDB::MONGODB_DB_NAME][RedisMongoDB::MONGODB_COLLECTION_NAME];

    bsoncxx::builder::basic::document filter_builder;
    mongocxx::options::find find_opts;

    if (node.targets.empty()) {
        filter_builder.append(bsoncxx::builder::basic::kvp(
            RedisMongoDB::MONGODB_FIELD_NAME[atomdb::MONGODB_FIELD::NAME], node.name));
    } else {
        bsoncxx::builder::basic::array children_array;
        for (auto& child : node.targets) {
            children_array.append(resolve_node_handle(child));
        }
        filter_builder.append(bsoncxx::builder::basic::kvp(
            RedisMongoDB::MONGODB_FIELD_NAME[atomdb::MONGODB_FIELD::TARGETS],
            bsoncxx::builder::basic::make_document(
                bsoncxx::builder::basic::kvp("$all", children_array.view()))));

        bsoncxx::builder::basic::document proj_builder;
        proj_builder.append(bsoncxx::builder::basic::kvp(
            RedisMongoDB::MONGODB_FIELD_NAME[atomdb::MONGODB_FIELD::ID], 1));
        find_opts.projection(proj_builder.view());
    }

    auto result = collection.find_one(filter_builder.view(), find_opts);
    if (!result) return "";

    auto id_element = (*result)[RedisMongoDB::MONGODB_FIELD_NAME[atomdb::MONGODB_FIELD::ID]];
    return id_element.get_string().value.data();
}
shared_ptr<atomdb_api_types::HandleList> MorkMongoDB::query_for_targets(shared_ptr<char> link_handle) {
    return query_for_targets(link_handle.get());
}
// WIP
shared_ptr<atomdb_api_types::HandleList> MorkMongoDB::query_for_targets(char* link_handle_ptr) {
    return NULL;
}
// <--