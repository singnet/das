#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "AtomDB.h"
#include "BoundedSharedQueue.h"
#include "MettaFileWriter.h"
#include "processor/ThreadPool.h"

#define BATCH_SIZE 100000
#define THREAD_POOL_SIZE 8

using namespace atomdb;
using namespace std;
using namespace commons;
using namespace atoms;
using namespace db_adapter;
using namespace processor;

namespace db_adapter {
class AtomPersister {
   public:
    AtomPersister(shared_ptr<BoundedSharedQueue> input_queue,
                  ThreadPool& pool,
                  shared_ptr<AtomDB> atomdb,
                  size_t batch_size = BATCH_SIZE,
                  bool save_metta_expression = false,
                  string metta_output_dir = "./knowledge_base",
                  size_t max_pending_batches = 4);
    ~AtomPersister();

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