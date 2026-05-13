#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "AtomDB.h"
#include "BoundedSharedQueue.h"
#include "DedicatedThread.h"
#include "JsonConfig.h"
#include "MettaFileWriter.h"
#include "PostgresWrapper.h"
#include "Processor.h"
#include "processor/ThreadPool.h"

#define BATCH_SIZE 100000
#define THREAD_POOL_SIZE 8

using namespace atomdb;
using namespace std;
using namespace commons;
using namespace atoms;
using namespace db_adapter;

namespace db_adapter {

struct MappingTask {
    string task_name;
    optional<string> context;
};

class DatabaseMappingStrategy {
   public:
    virtual ~DatabaseMappingStrategy() = default;
    virtual vector<MappingTask> create_tasks(const JsonConfig& config) = 0;
    virtual void execute_task(const MappingTask& task) = 0;
};

class DatabaseMappingOrchestrator : public ThreadMethod {
   public:
    DatabaseMappingOrchestrator(const JsonConfig& config, shared_ptr<BoundedSharedQueue> output_queue);
    virtual ~DatabaseMappingOrchestrator();

    bool thread_one_step() override;
    bool is_finished() const;

   private:
    void database_setup(const JsonConfig& config, shared_ptr<BoundedSharedQueue> output_queue);

    unique_ptr<DatabaseConnection> db_conn;
    unique_ptr<DatabaseMappingStrategy> strategy;

    vector<MappingTask> tasks;
    size_t current_task = 0;
    bool finished = false;
    bool initialized = false;
};

class PostgresMappingStrategy : public DatabaseMappingStrategy {
   public:
    explicit PostgresMappingStrategy(unique_ptr<PostgresWrapper> wrapper);
    vector<MappingTask> create_tasks(const JsonConfig& config) override;
    void execute_task(const MappingTask& task) override;

   private:
    unique_ptr<PostgresWrapper> wrapper;
};

class MultiThreadAtomPersister {
   public:
    MultiThreadAtomPersister(shared_ptr<BoundedSharedQueue> input_queue,
                             ThreadPool& pool,
                             shared_ptr<AtomDB> atomdb,
                             size_t batch_size = BATCH_SIZE,
                             bool save_metta_expression = false,
                             string metta_output_dir = "./knowledge_base",
                             size_t max_pending_batches = 4);
    ~MultiThreadAtomPersister();

    void dispatch();
    void set_producer_finished();
    bool is_producer_finished() const;
    int get_total_count() const;

   private:
    shared_ptr<BoundedSharedQueue> input_queue;
    ThreadPool& pool;
    shared_ptr<AtomDB> atomdb;
    size_t batch_size;
    atomic<bool> save_metta_expression;
    size_t max_pending_batches;
    shared_ptr<MettaFileWriter> metta_writer;

    atomic<bool> producer_finished{false};
    atomic<int> total_count{0};
    atomic<int> batches_dispatched{0};
    atomic<int> batches_completed{0};
    atomic<int> batches_failed{0};

    vector<Atom*> accumulator;
    void drain_into_accumulator();
    void flush_batch();
    void send_batch(vector<Atom*> atoms, int batch_id, shared_ptr<MettaFileWriter> writer);
    bool is_save_metta() const { return this->save_metta_expression; }
};

}  // namespace db_adapter
