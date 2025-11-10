#include "MorkDB.h"

#include <algorithm>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "HandleDecoder.h"
#include "MettaMapping.h"
#include "MettaParser.h"
#include "MettaParserActions.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace atoms;
using namespace commons;
using namespace metta;

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

// --> MorkDB : RedisMongoDB(context, skip_redis = true)
MorkDB::MorkDB(const string& context) : RedisMongoDB(context, true) {
    bool disable_cache = (Utils::get_environment("DAS_DISABLE_ATOMDB_CACHE") == "true");
    this->atomdb_cache = disable_cache ? nullptr : AtomDBCacheSingleton::get_instance();
    mork_setup();
}

MorkDB::~MorkDB() {}

bool MorkDB::allow_nested_indexing() { return true; }

void MorkDB::mork_setup() {
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

class MorkDBDecoder : public HandleDecoder {
   public:
    MorkDBDecoder(const string& raw_expr) {
        this->parser_actions = make_shared<MettaParserActions>();
        MettaParser parser(raw_expr, this->parser_actions);
        parser.parse(true);
    }

    ~MorkDBDecoder() {}

    shared_ptr<Atom> get_atom(const string& handle) override {
        return this->parser_actions->handle_to_atom[handle];
    }

    string get_metta_expression_handle() { return this->parser_actions->metta_expression_handle; }

    map<string, string> get_metta_expressions() {
        return this->parser_actions->handle_to_metta_expression;
    }

    shared_ptr<MettaParserActions> parser_actions;
};

shared_ptr<atomdb_api_types::HandleSet> MorkDB::query_for_pattern(const LinkSchema& link_schema) {
    if (this->atomdb_cache != nullptr) {
        auto cache_result = this->atomdb_cache->query_for_pattern(link_schema.handle());
        if (cache_result.is_cache_hit) return cache_result.result;
    }
    string pattern_metta = link_schema.metta_representation(*this);
    // template should equals to pattern_metta
    LOG_DEBUG("Fetching data...");
    vector<string> raw_expressions = this->mork_client->get(pattern_metta, pattern_metta);
    LOG_DEBUG("Fetching data...OK");
    auto handle_set = make_shared<atomdb_api_types::HandleSetMork>();

    Assignment assignments;
    LinkSchema local_schema(link_schema);

    for (const auto& raw_expr : raw_expressions) {
        MorkDBDecoder morkdb_decoder(raw_expr);

        string handle = morkdb_decoder.get_metta_expression_handle();
        auto metta_expressions = morkdb_decoder.get_metta_expressions();

        if (!local_schema.match(handle, assignments, morkdb_decoder)) {
            LOG_ERROR("[MORKDB] No match!");
            continue;
        }

        LOG_DEBUG("[MORKDB] assignments[" << handle << "]: " << assignments.to_string());

        handle_set->append(
            make_shared<atomdb_api_types::HandleSetMork>(handle, metta_expressions, assignments));
        assignments.clear();
    }

    if (this->atomdb_cache != nullptr)
        this->atomdb_cache->add_pattern_matching(link_schema.handle(), handle_set);

    return handle_set;
}

shared_ptr<atomdb_api_types::HandleList> MorkDB::query_for_targets(const string& handle) {
    auto document = this->get_atom_document(handle);
    if (document == nullptr || !document->contains(MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS])) {
        return nullptr;
    }
    return make_shared<atomdb_api_types::HandleListMork>(document);
}

vector<string> MorkDB::add_links(const vector<atoms::Link*>& links, bool throw_if_exists) {
    if (links.empty()) {
        return {};
    }

    vector<string> handles;

    for (const auto& link : links) {
        if (throw_if_exists && link_exists(link->handle())) {
            Utils::error("Link already exists: " + link->handle());
            return {};
        }

        auto unique_targets = set<string>(link->targets.begin(), link->targets.end());
        auto existing_targets =
            get_atom_documents(vector<string>(unique_targets.begin(), unique_targets.end()),
                               {MONGODB_FIELD_NAME[MONGODB_FIELD::ID]});
        if (existing_targets.size() != unique_targets.size()) {
            Utils::error("Failed to insert link: " + link->handle() + " has " +
                         to_string(unique_targets.size() - existing_targets.size()) +
                         " missing targets");
            return {};
        }

        auto metta_expression = link->metta_representation(*this);
        this->mork_client->post(metta_expression, "$x", "$x");

        auto mongodb_doc = atomdb_api_types::MongodbDocument(link, *this);
        this->upsert_document(mongodb_doc.value(), MONGODB_LINKS_COLLECTION_NAME);

        handles.push_back(link->handle());
    }

    return handles;
}

bool MorkDB::delete_link(const string& handle, bool delete_targets) {
    Utils::error("MORKDB does not support deleting links.", false);
    return false;
}

// <--
