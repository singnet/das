#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "ContextLoader.h"
#include "DataTypes.h"
#include "DedicatedThread.h"
#include "Pipeline.h"
#include "Processor.h"
#include "Utils.h"
#include "processor/ThreadPool.h"

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
         vector<string> queries_SQL,
         MAPPER_TYPE mapper_type) {
    LOG_DEBUG("Starting database adapter with the following parameters:");

    auto queue = make_shared<SharedQueue>();

    DatabaseMappingJob db_mapping_job(host, port, database, username, password, mapper_type, queue);
    auto producer = make_shared<DedicatedThread>("producer", &db_mapping_job);
    if (!tables_mapping.empty()) {
        for (const auto& table_mapping : tables_mapping) {
            db_mapping_job.add_task_table(table_mapping);
        }
    }
    LOG_DEBUG("Loaded " + to_string(tables_mapping.size()) + " table mappings from context file.");
    if (!queries_SQL.empty()) {
        for (size_t i = 0; i < queries_SQL.size(); i++) {
            db_mapping_job.add_task_query("custom_query_" + to_string(i), queries_SQL[i]);
        }
    }
    LOG_DEBUG("Loaded " + to_string(queries_SQL.size()) + " queries from query file.");

    /*
    DedicatedThread approach:
    */

    AtomPersistenceJob atomdb_job(queue);
    auto consumer = make_shared<DedicatedThread>("consumer", &atomdb_job);

    consumer->setup();
    consumer->start();
    LOG_DEBUG("Consumer thread started.");

    producer->setup();
    producer->start();
    LOG_DEBUG("Producer thread started.");

    while (!db_mapping_job.is_finished()) {
        Utils::sleep();
    }

    LOG_INFO("Mapping completed. Loading data into DAS.");
    producer->stop();

    atomdb_job.set_producer_finished();

    while (!atomdb_job.is_finished()) {
        LOG_DEBUG("Waiting for AtomPersistenceJob to finish...");
        Utils::sleep();
    }

    LOG_INFO("Loading completed!");
    consumer->stop();

    /*
    ThreadPool approach:
    */

    // AtomPersistenceJob2 atomdb_job(queue);
    // int num_consumers = 8;
    // ThreadPool pool("consumers_pool", num_consumers);
    // pool.setup();
    // pool.start();
    // for (int i = 0; i < num_consumers; ++i) {
    //     pool.enqueue([&atomdb_job]() { atomdb_job.consumer_task(); });
    // }

    // producer->setup();
    // producer->start();
    // LOG_DEBUG("Producer thread started.");
    // while (!db_mapping_job.is_finished()) {
    //     Utils::sleep();
    // }
    // LOG_INFO("Mapping completed. Loading data into DAS.");
    // producer->stop();
    // atomdb_job.set_producer_finished();

    // pool.wait();
    // pool.stop();
    // LOG_INFO("Loading completed!");
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
    string query_file_path = (argc >= 8) ? argv[7] : ".";
    string mapper_type_str = (argc >= 9) ? argv[8] : ".";

    if (is_empty_arg(context_file_path) && is_empty_arg(query_file_path)) {
        cerr << "Error: You must provide at least context_file_path OR query_file_path.\n\n";
        usage(argv[0]);
    }

    bool has_context = !is_empty_arg(context_file_path);
    bool has_query = !is_empty_arg(query_file_path);
    bool has_mapper = !is_empty_arg(mapper_type_str);

    vector<TableMapping> tables_mapping;
    if (has_context) {
        tables_mapping = ContextLoader::load_context_file(context_file_path);
    }

    vector<string> queries_SQL;
    if (has_query) {
        LOG_DEBUG("Loading queries from file: " + query_file_path);
        queries_SQL = ContextLoader::load_query_file(query_file_path);
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

    AtomDBSingleton::init();

    run(host, port, database, username, password, tables_mapping, queries_SQL, mapper_type);

    return 0;
}