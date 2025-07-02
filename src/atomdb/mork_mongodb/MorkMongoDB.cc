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
using namespace atomspace;
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
    LOG_DEBUG("POST request sent to MORK: "
              << " url=" << path << " data=" << data);
    auto res = send_request("POST", path, data);
    return res->body;
}
vector<string> MorkClient::get(const string& pattern, const string& template_) {
    auto path = "/export/" + url_encode(pattern) + "/" + url_encode(template_);
    LOG_DEBUG("GET request sent to MORK: "
              << " url=" << path);
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
    bool disable_cache = (Utils::get_environment("DAS_DISABLE_ATOMDB_CACHE") == "true");
    this->atomdb_cache = disable_cache ? nullptr : AtomDBCacheSingleton::get_instance();
    mork_setup();
}
MorkMongoDB::~MorkMongoDB() {}

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
void MorkMongoDB::attention_broker_setup() {}
shared_ptr<atomdb_api_types::HandleSet> MorkMongoDB::query_for_pattern(
    const LinkTemplateInterface& link_template) {
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_pattern(link_template);
        if (cache_result.is_cache_hit) return cache_result.result;
    }
    // WIP - This method will be implemented in the LinkTemplate
    string pattern_metta = link_template.get_metta_expression();
    // template should equals to pattern_metta
    LOG_DEBUG("Fetching data...");
    vector<string> raw_expressions = this->mork_client->get(pattern_metta, pattern_metta);
    LOG_DEBUG("Fetching data...OK");
    auto handle_set = make_shared<atomdb_api_types::HandleSetMork>();

    for (const auto& raw_expr : raw_expressions) {
        auto tokens = tokenize_expression(raw_expr);
        size_t pos = 0;
        auto atom = parse_tokens_to_atom(tokens, pos);
        auto link = static_cast<const atomspace::Link*>(atom);
        auto handle = Link::compute_handle(link->type, link->targets);
        // LOG_INFO("expression: " << raw_expr << " | "
        //                         << "handle: " << handle);
        handle_set->append(make_shared<atomdb_api_types::HandleSetMork>(handle));
    }

    if (this->atomdb_cache != nullptr)
        this->atomdb_cache->add_pattern_matching(link_template.get_handle(), handle_set);

    return handle_set;
}
vector<string> MorkMongoDB::tokenize_expression(const string& expr) {
    vector<string> tokens;
    size_t i = 0;
    size_t expr_size = expr.size();

    while (i < expr_size) {
        char c = expr[i];

        if (isspace(static_cast<unsigned char>(c))) {
            ++i;
            continue;
        }

        if (c == '(' || c == ')') {
            tokens.push_back(string(1, c));
            ++i;
            continue;
        }

        // Literal Nodes
        if (c == '"') {
            size_t start = i++;
            // move forward until you find the next " that is not escaped
            while (i < expr_size && (expr[i] != '"' || expr[i - 1] == '\\')) {
                ++i;
            }
            tokens.push_back(expr.substr(start, i - start + 1));
            ++i;
            continue;
        }

        // Non-literal Nodes
        size_t start = i;
        while (i < expr_size && !isspace(static_cast<unsigned char>(expr[i])) && expr[i] != '(' &&
               expr[i] != ')') {
            ++i;
        }
        tokens.push_back(expr.substr(start, i - start));
    }

    return tokens;
}
const commons::Atom* MorkMongoDB::parse_tokens_to_atom(const vector<string>& tokens, size_t& pos) {
    vector<const commons::Atom*> children;

    if (tokens[pos] == "(") {
        ++pos;
        while (pos < tokens.size() && tokens[pos] != ")") {
            if (tokens[pos] == "(") {
                children.push_back(parse_tokens_to_atom(tokens, pos));
            } else {
                const Atom* leaf = new Node("Symbol", tokens[pos++]);
                children.push_back(leaf);
            }
        }
        if (pos < tokens.size() && tokens[pos] == ")") ++pos;
        return new Link("Expression", children);
    } else {
        return new Node("Symbol", tokens[pos++]);
    }
}
shared_ptr<atomdb_api_types::HandleList> MorkMongoDB::query_for_targets(const string& handle) {
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_targets(handle);
        if (cache_result.is_cache_hit) return cache_result.result;
    }

    auto conn = this->mongodb_pool->acquire();
    auto collection = (*conn)[RedisMongoDB::MONGODB_DB_NAME][RedisMongoDB::MONGODB_COLLECTION_NAME];

    bsoncxx::builder::basic::document filter_builder;
    filter_builder.append(bsoncxx::builder::basic::kvp(
        RedisMongoDB::MONGODB_FIELD_NAME[atomdb::MONGODB_FIELD::ID], handle));

    mongocxx::options::find find_opts;
    bsoncxx::builder::basic::document proj_builder;
    proj_builder.append(bsoncxx::builder::basic::kvp(
        RedisMongoDB::MONGODB_FIELD_NAME[atomdb::MONGODB_FIELD::TARGETS], 1));
    find_opts.projection(proj_builder.view());

    auto result = collection.find_one(filter_builder.view(), find_opts);

    if (!result) {
        if (this->atomdb_cache != nullptr) this->atomdb_cache->add_handle_list(handle, nullptr);
        return nullptr;
    }

    // Extract the "targets" and convert it to vector<string>
    vector<string> targets;
    auto targets_element = (*result)[RedisMongoDB::MONGODB_FIELD_NAME[atomdb::MONGODB_FIELD::TARGETS]];
    if (targets_element && targets_element.type() == bsoncxx::type::k_array) {
        for (auto& v : targets_element.get_array().value) {
            targets.emplace_back(v.get_string().value.data());
        }
    }

    auto handle_list = make_shared<atomdb_api_types::HandleListMork>(targets);

    if (this->atomdb_cache != nullptr) this->atomdb_cache->add_handle_list(handle, handle_list);

    return handle_list;
}
// <--