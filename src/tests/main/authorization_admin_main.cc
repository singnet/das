#include <signal.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "AuthorizationManager.h"
#include "JsonConfig.h"
#include "MongoAuthorizationPersistence.h"
#include "Utils.h"
#include "nlohmann/json.hpp"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

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
         << "  --permission <read|write> \\\n"
         << "  --tokens <tokens_string> \\\n"
         << "  --config <path_to_config.json>\n\n"
         << "Example:\n"
         << "  " << prog_name << " authorize \\\n"
         << "    --public-key \"ssh-ed25519 AAAA... name@example.com\" \\\n"
         << "    --permission read \\\n"
         << "    --tokens \"LINK_TEMPLATE Expression 3 NODE Symbol Similarity\" \\\n"
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

int main(int argc, char* argv[]) {
    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    string action;
    string public_key;
    string permission;
    string tokens_str;
    string config_path;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];

        if (arg == "authorize" || arg == "revoke") {
            action = arg;
        } else if (arg == "--action" && i + 1 < argc) {
            action = argv[++i];
        } else if (arg == "--public-key" && i + 1 < argc) {
            public_key = argv[++i];
        } else if (arg == "--permission" && i + 1 < argc) {
            permission = argv[++i];
        } else if (arg == "--tokens" && i + 1 < argc) {
            tokens_str = argv[++i];
        } else if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        }
    }

    if (action.empty() || public_key.empty() || permission.empty() || tokens_str.empty() ||
        config_path.empty()) {
        cerr << "Error: Missing required arguments.\n\n";
        usage(argv[0]);
    }

    if (action != "authorize" && action != "revoke") {
        cerr << "Error: action must be 'authorize' or 'revoke'.\n\n";
        usage(argv[0]);
    }

    if (permission != "read" && permission != "write") {
        cerr << "Error: --permission must be 'read' or 'write'.\n\n";
        exit(1);
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
        make_shared<MongoAuthorizationPersistence>(endpoint, username, password, database, collection);
    auto manager = make_shared<AuthorizationManager>(persistence);

    vector<string> tokens = parse_tokens(tokens_str);

    bool read;
    bool write;
    if (permission == "read") {
        read = true;
        write = false;
    } else if (permission == "write") {
        read = false;
        write = true;
    }

    auto entry = atomdb_api_types::AccessPermissionEntry(tokens, read, write);

    if (action == "authorize") {
        manager->authorize(public_key, entry);
    } else if (action == "revoke") {
        manager->revoke(public_key, entry);
    }

    LOG_INFO("Admin finished successfully.");

    return 0;
}