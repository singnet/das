#include <signal.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "AuthorizationTypes.h"
#include "JsonConfig.h"
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
    cerr << "Usage: " << prog_name << " <authorize|revoke> \\\n"
         << "  --public-key <key_string> \\\n"
         << "  --link-template <tokens_string> \\\n"
         << "  --permission <read|write|read-write> \\\n"
         << "  --config <path_to_config.json>\n\n"
         << "Example:\n"
         << "  " << prog_name << " authorize \\\n"
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
         << "    --full-access\\\n"
         << "    --config config.json\n";
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

        if (arg == "authorize" || arg == "revoke") {
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

    if (full_access) {
        if (action.empty() || config_path.empty()) {
            cerr << "Error: Missing required arguments for full access.\n\n";
            exit(1);
        }
    } else {
        if (action.empty() || public_key.empty() || permissions.empty() || link_templates.empty() ||
            config_path.empty()) {
            cerr << "Error: Missing required arguments.\n\n";
            usage(argv[0]);
        }

        if (action != "authorize" && action != "revoke") {
            cerr << "Error: action must be 'authorize' or 'revoke'.\n\n";
            usage(argv[0]);
        }

        if (link_templates.size() != permissions.size()) {
            cerr << "Error: The number of --link-template arguments must match the number of "
                    "--permission "
                    "arguments.\n";
            exit(1);
        }
    }

    LOG_INFO("Starting Admin...");

    ifstream config_file(config_path);
    if (!config_file.good()) {
        cerr << "Error: Cannot open config file: " << config_path << endl;
        exit(1);
    }
    stringstream config_buffer;
    config_buffer << config_file.rdbuf();
    JsonConfig json_config = JsonConfig(nlohmann::json::parse(config_buffer.str()));

    Utils::init_random(0);

    string endpoint = json_config.at_path("mongodb.endpoint").get_or<string>("");
    string username = json_config.at_path("mongodb.username").get_or<string>("");
    string password = json_config.at_path("mongodb.password").get_or<string>("");
    string database = json_config.at_path("mongodb.database_name").get_or<string>("");
    string collection = json_config.at_path("mongodb.collection_name").get_or<string>("");

    auto persistence =
        make_shared<MongodbAuthorizationPersistence>(endpoint, username, password, database, collection);

    if (action == "authorize") {
        if (full_access) {
            persistence->authorize(public_key);
        } else {
            vector<pair<LinkSchema, unsigned int>> schemas = build_schemas(link_templates, permissions);
            persistence->authorize(public_key, schemas);
        }
    } else if (action == "revoke") {
        persistence->revoke(public_key);
    }

    LOG_INFO("Admin finished successfully.");

    return 0;
}