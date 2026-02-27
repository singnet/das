#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "ContextLoader.h"
#include "DataTypes.h"
#include "DedicatedThread.h"
#include "Pipeline.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace commons;
using namespace atomdb;
using namespace db_adapter;
using namespace processor;

void ctrl_c_handler(int) {
    std::cout << "Stopping database adapter..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

void usage(const char* a) {
    cerr << "Usage: " << a
         << " <host> <port> <database> <username> [password] "
            "[context_file_path] [query_SQL] [mapper_type] \n\n"
            "Rules:\n"
            "- You must provide at least context_file_path OR query_SQL.\n"
            "- Use '.' to skip an optional argument.\n\n"
            "Examples:\n"
            "  ./bin host port db user pwd ./context.json\n"
            "  ./bin host port db user pwd . \"SELECT * FROM table\"\n"
            "  ./bin host port db user pwd ./context.json . ATOMS\n"
            "  ./bin host port db user pwd . \"SELECT * FROM table\" ATOMS\n"
            "  ./bin host port db user pwd ./context.json \"SELECT * FROM table\" METTA\n";
    exit(1);
}

bool is_empty_arg(const string& s) { return s.empty() || s == "."; }

void run(string host,
         int port,
         string database,
         string username,
         string password,
         vector<TableMapping> tables_mapping,
         string query_SQL,
         MAPPER_TYPE mapper_type) {
    auto queue = make_shared<SharedQueue>();

    DatabaseMappingJob db_mapping_job(host, port, database, username, password, mapper_type, queue);
    auto producer = make_shared<DedicatedThread>("producer", &db_mapping_job);
    if (!tables_mapping.empty()) {
        for (const auto& table_mapping : tables_mapping) {
            db_mapping_job.add_task_table(table_mapping);
        }
    }
    if (!query_SQL.empty()) {
        db_mapping_job.add_task_query("custom_query", query_SQL);
    }

    AtomPersistenceJob atomdb_job(queue);
    auto consumer = make_shared<DedicatedThread>("consumer", &atomdb_job);

    consumer->setup();
    consumer->start();

    producer->setup();
    producer->start();

    while (!db_mapping_job.is_finished()) {
        Utils::sleep();
    }

    LOG_INFO("Mapping completed. Loading data into DAS.");
    producer->stop();

    while (!atomdb_job.is_finished()) {
        LOG_DEBUG("Waiting for AtomPersistenceJob to finish...");
        Utils::sleep();
    }

    LOG_INFO("Loading completed!");
    consumer->stop();
}

int main(int argc, char* argv[]) {
    if (argc < 7 || argc > 9) {
        usage(argv[0]);
    }

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);

    string host = argv[1];
    int port = stoi(argv[2]);
    string database = argv[3];
    string username = argv[4];
    string password = argv[5];

    string context_file_path = argv[6];
    string query_SQL = (argc >= 8) ? argv[7] : ".";
    string mapper_type_str = (argc >= 9) ? argv[8] : ".";

    if (is_empty_arg(context_file_path) && is_empty_arg(query_SQL)) {
        cerr << "Error: You must provide at least context_file_path OR query_SQL.\n\n";
        usage(argv[0]);
    }

    bool has_context = !is_empty_arg(context_file_path);
    bool has_query = !is_empty_arg(query_SQL);
    bool has_mapper = !is_empty_arg(mapper_type_str);

    vector<TableMapping> tables_mapping;
    if (has_context) {
        tables_mapping = ContextLoader::load_context_file(context_file_path);
    }

    if (!has_query) {
        query_SQL.clear();
    }

    MAPPER_TYPE mapper_type = MAPPER_TYPE::SQL2ATOMS;
    ;

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

    AtomDBSingleton::init();

    run(host, port, database, username, password, tables_mapping, query_SQL, mapper_type);

    return 0;
}