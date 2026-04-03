#include "DatabaseAdapterRunner.h"

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

DatabaseAdapterRunner::DatabaseAdapterRunner(const string& host,
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

void DatabaseAdapterRunner::run() {
    auto queue = make_shared<BoundedSharedQueue>(100000);

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

    unsigned int num_threads = 8;
    ThreadPool pool("consumers_pool", num_threads);
    pool.setup();
    pool.start();

    BatchConsumer consumer(queue, pool, BATCH_SIZE);

    producer->setup();
    producer->start();
    LOG_DEBUG("Producer thread started.");

    while (true) {
        consumer.dispatch();

        if (db_mapping_job.is_finished() && !consumer.is_producer_finished()) {
            LOG_INFO("Mapping completed. Loading data into DAS.");
            producer->stop();
            consumer.set_producer_finished();
        }

        if (consumer.is_producer_finished() && queue->empty()) {
            consumer.dispatch();
            break;
        }

        if (queue->empty()) {
            this_thread::sleep_for(chrono::milliseconds(5));
        }
    }

    pool.wait();
    pool.stop();
    LOG_INFO("Loading completed! Total atoms: " << consumer.get_total_count());
}