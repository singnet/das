#include <signal.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "AuthorizationTypes.h"
#include "Hasher.h"
#include "JsonConfig.h"
#include "JsonConfigParser.h"
#include "LinkSchema.h"
#include "MongodbAuthorizationPersistence.h"
#include "Utils.h"
#include "nlohmann/json.hpp"

using namespace std;
using namespace commons;
using namespace atomdb;

void ctrl_c_handler(int) {
    std::cout << "\nStopping admin..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

void usage(const char* prog_name) {
    cerr << "Usage: " << prog_name << " <grant|revoke> \\\n"
         << "  --public-key <key_string> \\\n"
         << "  --link-template <tokens_string> \\\n"
         << "  --permission <read|write|read-write> \\\n"
         << "  --config <path_to_config.json>\n\n"
         << "Example with link templates:\n"
         << "  " << prog_name << " grant \\\n"
         << "    --public-key \"ssh-ed25519 AAAA... name@example.com\" \\\n"
         << "    --link-template \"LINK_TEMPLATE Expression 3 NODE Symbol Similarity NODE Symbol "
            "\\\"human\\\" VARIABLE V2\" \\\n"
         << "    --permission read \\\n"
         << "    --link-template \"LINK_TEMPLATE Expression 3 NODE Symbol Similarity NODE Symbol "
            "\\\"monkey\\\" VARIABLE V2\" \\\n"
         << "    --permission read \\\n"
         << "    --link-template \"LINK_TEMPLATE Expression 3 NODE Symbol Inheritance VARIABLE V1 NODE "
            "Symbol \\\"mammal\\\"\" \\\n"
         << "    --permission read-write \\\n"
         << "    --config config.json\n\n"
         << "Example with full access:\n"
         << "  " << prog_name << " grant \\\n"
         << "    --public-key \"ssh-ed25519 AAAA... name@example.com\" \\\n"
         << "    --full-access \\\n"
         << "    --config config.json\n\n";
    exit(1);
}

vector<string> parse_tokens(const string& tokens_str) {
    vector<string> tokens;
    stringstream ss(tokens_str);
    string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

vector<pair<LinkSchema, unsigned int>> build_schemas(const vector<string>& link_templates,
                                                     const vector<string>& permissions) {
    vector<pair<LinkSchema, unsigned int>> schemas;

    for (size_t i = 0; i < link_templates.size(); ++i) {
        vector<string> tokens = parse_tokens(link_templates[i]);
        LinkSchema schema(tokens);

        auto permission = permissions[i];

        if (permission == "read") {
            schemas.emplace_back(schema, 1);
        } else if (permission == "write") {
            schemas.emplace_back(schema, 2);
        } else if (permission == "read-write") {
            schemas.emplace_back(schema, 3);
        } else {
            cerr << "Error: --permission must be 'read', 'write' or 'read-write'.\n\n";
            exit(1);
        }
    }
    return schemas;
}

void create_config_collection(shared_ptr<MongodbAuthorizationPersistence> persistence,
                              const string& database,
                              const string& config_collection) {
    using bsoncxx::v_noabi::builder::basic::kvp;
    using bsoncxx::v_noabi::builder::basic::make_document;

    auto mongodb_pool = persistence->get_mongodb_pool();
    auto conn = mongodb_pool->acquire();
    auto collection = (*conn)[database][config_collection];

    auto config_id = Hasher::plain_string_hash(config_collection);

    if (!collection.find_one(make_document(kvp("_id", config_id)))) {
        collection.insert_one(make_document(kvp("_id", config_id), kvp("protected", true)));
    }
}

int main(int argc, char* argv[]) {
    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    string action;
    string public_key;
    vector<string> permissions;
    vector<string> link_templates;
    string config_path;
    bool full_access = false;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];

        if (arg == "grant" || arg == "revoke") {
            action = arg;
        } else if (arg == "--action" && i + 1 < argc) {
            action = argv[++i];
        } else if (arg == "--public-key" && i + 1 < argc) {
            public_key = argv[++i];
        } else if (arg == "--permission" && i + 1 < argc) {
            permissions.push_back(argv[++i]);
        } else if (arg == "--link-template" && i + 1 < argc) {
            link_templates.push_back(argv[++i]);
        } else if (arg == "--full-access") {
            full_access = true;
        } else if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        }
    }

    if (action != "grant" && action != "revoke") {
        cerr << "\nError: action must be 'grant' or 'revoke'.\n\n";
        usage(argv[0]);
    }

    if (full_access) {
        if (public_key.empty() || config_path.empty()) {
            cerr << "\nError: Missing required arguments for full access.\n\n";
            usage(argv[0]);
        }
    } else {
        if (action.empty() || public_key.empty() || permissions.empty() || link_templates.empty() ||
            config_path.empty()) {
            cerr << "\nError: Missing required arguments.\n\n";
            usage(argv[0]);
        }
        if (link_templates.size() != permissions.size()) {
            cerr << "\nError: The number of --link-template arguments must match the number of "
                    "--permission arguments.\n\n";
            usage(argv[0]);
        }
    }

    cout << "Starting Admin..." << endl;

    JsonConfig json_config = JsonConfigParser::load(config_path);

    Utils::init_random(0);

    string endpoint = json_config.at_path("atomdb.mongodb.endpoint").get_or<string>("");
    string username = json_config.at_path("atomdb.mongodb.username").get_or<string>("");
    string password = json_config.at_path("atomdb.mongodb.password").get_or<string>("");

    string prefix = json_config.at_path("prefix").get_or<string>("");
    string database = prefix + "das";
    string access_permissions_collection = prefix + "access_permissions";
    string config_collection = prefix + "config";

    auto persistence = make_shared<MongodbAuthorizationPersistence>(
        endpoint, username, password, database, access_permissions_collection);

    if (action == "grant") {
        if (full_access) {
            persistence->grant_unrestricted(public_key);
        } else {
            vector<pair<LinkSchema, unsigned int>> schemas = build_schemas(link_templates, permissions);
            persistence->grant(public_key, schemas);
        }
    } else if (action == "revoke") {
        persistence->revoke(public_key);
    }

    create_config_collection(persistence, database, config_collection);

    cout << "Admin finished successfully." << endl;

    return 0;
}