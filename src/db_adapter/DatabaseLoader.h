#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "AtomDBSingleton.h"
#include "BoundedSharedQueue.h"
#include "DedicatedThread.h"
#include "PostgresWrapper.h"
#include "Processor.h"
#include "processor/ThreadPool.h"

#define BATCH_SIZE 100000

using namespace atomdb;
using namespace std;
using namespace commons;
using namespace atoms;
using namespace db_adapter;

namespace db_adapter {

class DatabaseMappingJob : public ThreadMethod {
   public:
    DatabaseMappingJob(const string& host,
                       int port,
                       const string& database,
                       const string& user,
                       const string& password,
                       MAPPER_TYPE mapper_type = MAPPER_TYPE::SQL2ATOMS,
                       shared_ptr<BoundedSharedQueue> output_queue = nullptr);
    ~DatabaseMappingJob();

    void add_task_query(const string& virtual_name, const string& query);
    void add_task_table(TableMapping table_mapping);

    bool thread_one_step() override;

    bool is_finished() const;

   protected:
    struct MappingTask {
        enum Type { TABLE, QUERY } type;
        TableMapping table_mapping;
        string virtual_name;
        string query;
    };
    unique_ptr<PostgresDatabaseConnection> db_conn;
    unique_ptr<PostgresWrapper> wrapper;
    vector<MappingTask> tasks;
    size_t current_task = 0;
    bool finished = false;
    bool initialized = false;
};

class AtomPersistenceJob : public ThreadMethod {
   public:
    AtomPersistenceJob(shared_ptr<BoundedSharedQueue> input_queue);
    ~AtomPersistenceJob();

    bool thread_one_step() override;
    bool is_finished() const;
    void set_producer_finished();

   protected:
    atomic<int> count = 0;
    vector<Atom*> atoms;
    shared_ptr<BoundedSharedQueue> input_queue;
    shared_ptr<AtomDB> atomdb;
    bool finished = false;
    bool producer_finished = false;
};

class BatchConsumer {
   public:
    BatchConsumer(shared_ptr<BoundedSharedQueue> input_queue,
                  ThreadPool& pool,
                  size_t batch_size = BATCH_SIZE,
                  size_t max_pending_batches = 4);
    ~BatchConsumer();

    void dispatch();
    void set_producer_finished();
    bool is_producer_finished() const;
    int get_total_count() const;

   private:
    shared_ptr<BoundedSharedQueue> input_queue;
    shared_ptr<AtomDB> atomdb;
    ThreadPool& pool;
    size_t batch_size;
    size_t max_pending_batches;

    atomic<bool> producer_finished{false};
    atomic<int> total_count{0};
    atomic<int> batches_dispatched{0};
    atomic<int> batches_completed{0};
    atomic<int> batches_failed{0};

    vector<Atom*> accumulator;
    void drain_into_accumulator();
    void flush_batch();
    void send_batch(vector<Atom*> atoms, int batch_id);
};

}  // namespace db_adapter
