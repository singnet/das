#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "ContextLoader.h"
#include "DatabaseAdapter.h"
#include "DatabaseTypes.h"
#include "JsonConfig.h"
#include "JsonConfigParser.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace commons;
using namespace atomdb;
using namespace db_adapter;

void ctrl_c_handler(int) {
    std::cout << "Stopping database adapter..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

void usage(const char* a) {
    cerr << "Usage: " << a
         << " <config_file> <host> <port> <database> <username> [password] "
            "[table_file_path] [query_file_path] [mapper_type] \n\n"
            "Rules:\n"
            "- You must provide at least table_file_path OR query_file_path.\n"
            "- Use '.' to skip an optional argument.\n\n"
            "Examples:\n"
            "  ./bin config.json host port db user pwd ./table.json\n"
            "  ./bin config.json host port db user pwd . ./query.sql\n"
            "  ./bin config.json host port db user pwd ./table.json . ATOMS\n"
            "  ./bin config.json host port db user pwd . ./query.sql ATOMS\n"
            "  ./bin config.json host port db user pwd ./table.json ./query.sql METTA\n";
    exit(1);
}

bool is_empty_arg(const string& s) { return s.empty() || s == "."; }

int main(int argc, char* argv[]) {
    if (argc < 8 || argc > 10) {
        usage(argv[0]);
    }

    LOG_INFO("Starting database adapter...");
    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    string config = argv[1];
    string host = argv[2];
    int port = stoi(argv[3]);
    string database = argv[4];
    string username = argv[5];
    string password = argv[6];
    string table_file_path = argv[7];
    string query_file_path = (argc >= 9) ? argv[8] : ".";
    string mapper_type_str = (argc >= 10) ? argv[9] : ".";

    if (is_empty_arg(table_file_path) && is_empty_arg(query_file_path)) {
        cerr << "Error: You must provide at least table_file_path OR query_file_path.\n\n";
        usage(argv[0]);
    }

    bool has_table = !is_empty_arg(table_file_path);
    bool has_query = !is_empty_arg(query_file_path);
    bool has_mapper = !is_empty_arg(mapper_type_str);

    vector<TableMapping> tables_mapping;
    if (has_table) {
        tables_mapping = ContextLoader::load_table_file(table_file_path);
    }

    vector<string> queries_SQL;
    if (has_query) {
        queries_SQL = ContextLoader::load_query_file(query_file_path);
        LOG_DEBUG("Loaded " + to_string(queries_SQL.size()) + " queries from file: " + query_file_path);
    }

    MAPPER_TYPE mapper_type = MAPPER_TYPE::SQL2ATOMS;

    if (has_mapper) {
        if (mapper_type_str == "ATOMS") {
            mapper_type = MAPPER_TYPE::SQL2ATOMS;
        } else if (mapper_type_str == "METTA") {
            mapper_type = MAPPER_TYPE::SQL2METTA;
        } else {
            cerr << "Error: Invalid mapper_type. Use 'ATOMS' or 'METTA'.\n\n";
            usage(argv[0]);
        }
    }

    JsonConfig json_config = JsonConfigParser::load(config);
    auto atomdb_config = json_config.at_path("atomdb").get_or<JsonConfig>(JsonConfig());
    AtomDBSingleton::init(atomdb_config);

    DatabaseAdapter db_adapter(
        host, port, database, username, password, tables_mapping, queries_SQL, mapper_type);
    db_adapter.run();

    return 0;
}