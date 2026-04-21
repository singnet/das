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

void db_adapter::run_database_adapter(const JsonConfig& config, MAPPER_TYPE mapper_type) {
    auto queue = make_shared<BoundedSharedQueue>();

    DatabaseMappingOrchestrator db_mapping_orchestrator(config, mapper_type, queue);
    auto producer = make_shared<DedicatedThread>("producer", &db_mapping_orchestrator);

    ThreadPool pool("consumers_pool", THREAD_POOL_SIZE);
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