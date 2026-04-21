#include "DatabaseAdapter.h"

#include <chrono>
#include <thread>

#include "AtomDBSingleton.h"
#include "BoundedSharedQueue.h"
#include "DatabaseLoader.h"
#include "DedicatedThread.h"
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

DatabaseAdapter::DatabaseAdapter(const string& host,
                                 int port,
                                 const string& database,
                                 const string& username,
                                 const string& password,
                                 const vector<TableMapping>& tables_mapping,
                                 const vector<string>& queries_SQL,
                                 MAPPER_TYPE mapper_type)
    : host(host),
      port(port),
      database(database),
      username(username),
      password(password),
      tables_mapping(tables_mapping),
      queries_SQL(queries_SQL),
      mapper_type(mapper_type) {}

void DatabaseAdapter::run() {
    auto queue = make_shared<BoundedSharedQueue>(100000);

    DatabaseMappingOrchestrator db_mapping_orchestrator(
        host, port, database, username, password, mapper_type, queue);

    auto producer = make_shared<DedicatedThread>("producer", &db_mapping_orchestrator);
    if (!this->tables_mapping.empty()) {
        for (const auto& table_mapping : this->tables_mapping) {
            db_mapping_orchestrator.add_task_table(table_mapping);
        }
    }
    LOG_DEBUG("Loaded " + to_string(this->tables_mapping.size()) + " table mappings from context file.");
    if (!this->queries_SQL.empty()) {
        for (size_t i = 0; i < this->queries_SQL.size(); i++) {
            db_mapping_orchestrator.add_task_query("custom_query_" + to_string(i), this->queries_SQL[i]);
        }
    }
    LOG_DEBUG("Loaded " + to_string(this->queries_SQL.size()) + " queries from query file.");

    unsigned int num_threads = 8;
    ThreadPool pool("consumers_pool", num_threads);
    pool.setup();
    pool.start();

    MultiThreadAtomPersister consumer(queue, pool, BATCH_SIZE);

    producer->setup();
    producer->start();
    LOG_DEBUG("Producer thread started.");

    while (true) {
        consumer.dispatch();

        if (db_mapping_orchestrator.is_finished() && !consumer.is_producer_finished()) {
            LOG_INFO("Mapping completed. Loading data into DAS.");
            producer->stop();
            consumer.set_producer_finished();
        }

        if (consumer.is_producer_finished() && queue->empty()) {
            consumer.dispatch();
            break;
        }

        if (queue->empty()) {
            Utils::sleep(5);
        }
    }

    pool.wait();
    pool.stop();
    LOG_INFO("Loading completed! Total atoms: " << consumer.get_total_count());
}