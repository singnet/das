#include <signal.h>

#include <iostream>
#include <string>

#include "Utils.h"
#include "ContextLoader.h"
#include "Pipeline.h"
#include "DedicatedThread.h"
#include "DataTypes.h"

using namespace std;
using namespace db_adapter;
using namespace processor;

void ctrl_c_handler(int) {
    std::cout << "Stopping database adapter..." << std::endl;
    std::cout << "Done." << std::endl;
    exit(0);
}

void run(string host, string port, string database, string username, string password, string context_file_path, MAPPER_TYPE mapper_type, string query_SQL) {
    auto queue = make_shared<SharedQueue>();

    DatabaseMappingJob db_mapping_job(host, port, database, username, password, mapper_type, queue);
    auto producer = make_shared<DedicatedThread>("producer", &db_mapping_job);

    AtomPersistenceJob atomdb_job(queue);
    auto consumer = make_shared<DedicatedThread>("consumer", &atomdb_job);

    consumer->setup();
    consumer->start();

    producer->setup();
    producer->start();

    while (!db_mapping_job.is_finished() || !atomdb_job.is_finished()) {
        Utils::sleep();
    }

    producer->stop();
    consumer->stop();
}

int main(int argc, char* argv[]) {
    if (argc != 7 && argc != 8) {
        cerr << "Usage: " << argv[0]
             << " <host> <port> <database> <username> <password> <context_file_path> [mapper_type] [query_SQL]"
             << endl;
        exit(1);
    }

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    
    string host = argv[1];
    string port = argv[2];
    string database = argv[3];
    string username = argv[4];
    string password = argv[5];
    string context_file_path = argv[6];
    MAPPER_TYPE mapper_type = (argc >= 8) ? argv[7] : MAPPER_TYPE::SQL2ATOMS;
    string query_SQL = (argc == 9) ? argv[8] : "";

    vector<TableMapping> tables_mapping = ContextLoader::load_context_file(context_file_path);


    run(host, port, database, username, password, context_file_path, mapper_type, query_SQL);

    return 0;
}