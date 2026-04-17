#include "DatabaseLoader.h"

#include "AtomDBSingleton.h"
#include "BoundedSharedQueue.h"
#include "DedicatedThread.h"
#include "Logger.h"
#include "PostgresWrapper.h"
#include "Processor.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace std;
using namespace commons;
using namespace atoms;
using namespace db_adapter;

DatabaseMappingJob::DatabaseMappingJob(const string& host,
                                       int port,
                                       const string& database,
                                       const string& user,
                                       const string& password,
                                       MAPPER_TYPE mapper_type,
                                       shared_ptr<BoundedSharedQueue> output_queue) {
    this->db_conn =
        make_unique<PostgresDatabaseConnection>("psql-conn", host, port, database, user, password);
    this->wrapper = make_unique<PostgresWrapper>(*db_conn, mapper_type, output_queue);
}

DatabaseMappingJob::~DatabaseMappingJob() { this->db_conn->stop(); }

void DatabaseMappingJob::add_task_query(const string& virtual_name, const string& query) {
    this->tasks.push_back(MappingTask{MappingTask::QUERY, TableMapping{}, virtual_name, query});
}

void DatabaseMappingJob::add_task_table(TableMapping table_mapping) {
    this->tasks.push_back(MappingTask{MappingTask::TABLE, move(table_mapping), "", ""});
}

bool DatabaseMappingJob::thread_one_step() {
    LOG_DEBUG("DatabaseMappingJob thread_one_step called. Current task index: " << this->current_task);
    if (this->current_task >= this->tasks.size()) {
        this->db_conn->stop();
        return false;
    }

    if (!this->initialized) {
        this->db_conn->setup();
        this->db_conn->start();
        this->initialized = true;
    }

    auto& task = this->tasks[this->current_task];

    LOG_DEBUG("Processing task " << this->current_task << " of type "
                                 << (task.type == MappingTask::TABLE ? "TABLE" : "QUERY"));

    if (task.type == MappingTask::TABLE) {
        auto table = this->wrapper->get_table(task.table_mapping.table_name);
        this->wrapper->map_table(table,
                                 task.table_mapping.where_clauses.value_or(vector<string>{}),
                                 task.table_mapping.skip_columns.value_or(vector<string>{}),
                                 false);
    } else if (task.type == MappingTask::QUERY) {
        this->wrapper->map_sql_query(task.virtual_name, task.query);
    }

    this->current_task++;
    this->finished = (this->current_task >= this->tasks.size());
    return !this->finished;
}

bool DatabaseMappingJob::is_finished() const { return this->finished; }

AtomPersistenceJob::AtomPersistenceJob(shared_ptr<BoundedSharedQueue> input_queue)
    : input_queue(input_queue) {
    this->atomdb = AtomDBSingleton::get_instance();
}

AtomPersistenceJob::~AtomPersistenceJob() {
    for (auto& atom : this->atoms) {
        delete atom;
    }
    this->atoms.clear();
}

bool AtomPersistenceJob::thread_one_step() {
    LOG_DEBUG("== START ==");
    LOG_DEBUG("Current input queue size 1: " << this->input_queue->size());
    LOG_DEBUG("Current finished status: " << (this->finished ? "true" : "false"));

    if (this->input_queue->empty()) {
        if (this->producer_finished) {
            LOG_INFO(
                "Producer has finished and input queue is empty. Marking AtomPersistenceJob as "
                "finished.");
            this->finished = true;
            if (!this->atoms.empty()) {
                LOG_DEBUG("Processing remaining " << this->atoms.size() << " atoms.");
                this->atomdb->add_atoms(this->atoms, false, true);
                LOG_DEBUG("Batch processed and added to AtomDB. Clearing batch from memory.");
                for (auto& atom : this->atoms) {
                    delete atom;
                }
                this->atoms.clear();
            }
        }
        Utils::sleep();
        return false;
    }

    try {
        for (size_t i = 0; i < BATCH_SIZE; ++i) {
            if (this->input_queue->empty()) break;
            auto atom = (Atom*) this->input_queue->dequeue();
            this->count++;
            if (atom == NULL) {
                Utils::error("Dequeued atom is NULL");
            }
            this->atoms.push_back(atom);
        }
        if (this->atoms.size() >= BATCH_SIZE) {
            LOG_DEBUG("Processing batch of " << this->atoms.size() << " atoms.");
            this->atomdb->add_atoms(this->atoms, false, true);
            LOG_DEBUG("Batch processed and added to AtomDB. Clearing batch from memory.");
            for (auto& atom : this->atoms) {
                delete atom;
            }
            this->atoms.clear();
        }
        LOG_DEBUG(this->count << " atoms removed from the Queue");
        LOG_DEBUG("Current Atom size: " << this->atoms.size());
        LOG_DEBUG("Current input queue size 2: " << this->input_queue->size());
        LOG_DEBUG("== END ==");
        return true;
    } catch (const exception& e) {
        Utils::error("Error in Worker: " + string(e.what()));
        for (auto& atom : this->atoms) {
            delete atom;
        }
        this->atoms.clear();
    }

    return false;
}

bool AtomPersistenceJob::is_finished() const { return this->finished; }

void AtomPersistenceJob::set_producer_finished() { this->producer_finished = true; }

/*
    BatchConsumer implementation using ThreadPool
*/

BatchConsumer::BatchConsumer(shared_ptr<BoundedSharedQueue> input_queue,
                             ThreadPool& pool,
                             size_t batch_size,
                             size_t max_pending_batches)
    : input_queue(input_queue),
      pool(pool),
      batch_size(batch_size),
      max_pending_batches(max_pending_batches) {
    this->atomdb = AtomDBSingleton::get_instance();
    this->accumulator.reserve(batch_size);
    LOG_INFO("BatchConsumer initialized | batch_size: " << batch_size << " | max_pending_batches: "
                                                        << max_pending_batches
                                                        << " | pool: " << this->pool.to_string());
}

BatchConsumer::~BatchConsumer() {
    LOG_INFO("BatchConsumer destroyed | total_atoms: "
             << this->total_count.load() << " | batches_dispatched: " << this->batches_dispatched.load()
             << " | batches_completed: " << this->batches_completed.load()
             << " | batches_failed: " << this->batches_failed.load());
}

void BatchConsumer::dispatch() {
    if (static_cast<size_t>(this->pool.size()) >= this->max_pending_batches) {
        this->flush_batch();
        return;
    }

    this->drain_into_accumulator();

    this->flush_batch();

    if (this->producer_finished.load() && !this->accumulator.empty()) {
        int batch_id = this->batches_dispatched.fetch_add(1) + 1;

        LOG_INFO("Dispatching FINAL batch #" << batch_id << " | size: " << this->accumulator.size()
                                             << " | (remainder < batch_size)");

        vector<Atom*> final_batch = move(this->accumulator);
        this->accumulator.clear();

        this->pool.enqueue(
            [this, b = move(final_batch), batch_id]() mutable { send_batch(move(b), batch_id); });
    }
}

void BatchConsumer::set_producer_finished() {
    this->producer_finished.store(true);
    LOG_INFO("Producer finished signal received"
             << " | accumulator_size: " << this->accumulator.size() << " | queue_remaining: "
             << this->input_queue->size() << " | total_atoms_so_far: " << this->total_count.load()
             << " | batches_dispatched: " << this->batches_dispatched.load()
             << " | batches_completed: " << this->batches_completed.load());
}

bool BatchConsumer::is_producer_finished() const { return this->producer_finished.load(); }

int BatchConsumer::get_total_count() const { return this->total_count.load(); }

void BatchConsumer::drain_into_accumulator() {
    size_t limit = (this->accumulator.size() < this->batch_size)
                       ? (this->batch_size - this->accumulator.size())
                       : 0;

    if (limit == 0) return;

    size_t drained = 0;

    while (drained < limit) {
        if (this->input_queue->empty()) break;

        void* raw_ptr = this->input_queue->dequeue();
        queue<Atom*>* batch_queue = static_cast<queue<Atom*>*>(raw_ptr);

        if (batch_queue != nullptr) {
            while (!batch_queue->empty()) {
                Atom* atom = batch_queue->front();
                batch_queue->pop();
                if (atom != nullptr) {
                    this->accumulator.push_back(atom);
                    ++drained;
                }
            }
            delete batch_queue;
        }
    }
}

void BatchConsumer::flush_batch() {
    while (this->accumulator.size() >= this->batch_size) {
        if (static_cast<size_t>(this->pool.size()) >= this->max_pending_batches) {
            LOG_DEBUG("flush_batch() | pool_pending_tasks: "
                      << this->pool.size() << " | accumulator_size: " << this->accumulator.size()
                      << " | waiting for pool to drain");
            return;
        }

        vector<Atom*> batch(this->accumulator.begin(), this->accumulator.begin() + this->batch_size);
        this->accumulator.erase(this->accumulator.begin(), this->accumulator.begin() + this->batch_size);

        int batch_id = this->batches_dispatched.fetch_add(1) + 1;

        LOG_DEBUG("Dispatching batch #" << batch_id << " | size: " << batch.size()
                                        << " | accumulator_remaining: " << this->accumulator.size()
                                        << " | queue_remaining: " << this->input_queue->size()
                                        << " | pool_pending_tasks: " << this->pool.size()
                                        << " | total_dispatched: " << this->batches_dispatched.load());

        this->pool.enqueue(
            [this, b = move(batch), batch_id]() mutable { send_batch(move(b), batch_id); });
    }
}

void BatchConsumer::send_batch(vector<Atom*> atoms, int batch_id) {
    StopWatch timer_success;
    StopWatch timer_failure;
    timer_success.start();
    timer_failure.start();

    LOG_DEBUG("Batch #" << batch_id << " started | size: " << atoms.size()
                        << " | thread: " << this_thread::get_id());

    try {
        this->atomdb->add_atoms(atoms, false, true);

        timer_success.stop();

        int new_total =
            this->total_count.fetch_add(static_cast<int>(atoms.size())) + static_cast<int>(atoms.size());
        this->batches_completed.fetch_add(1);

        LOG_INFO("Batch #" << batch_id << " completed | size: " << atoms.size()
                           << " | time: " << timer_success.milliseconds() << "ms"
                           << " | total_atoms_so_far: " << new_total << " | batches_completed: "
                           << this->batches_completed.load() << " | thread: " << this_thread::get_id());

    } catch (const exception& e) {
        timer_failure.stop();

        this->batches_failed.fetch_add(1);

        LOG_ERROR("Batch #" << batch_id << " FAILED | size: " << atoms.size()
                            << " | time: " << timer_failure.milliseconds() << "ms"
                            << " | error: " << e.what() << " | batches_failed: "
                            << this->batches_failed.load() << " | thread: " << this_thread::get_id());

        Utils::error("Error in batch #" + to_string(batch_id) + ": " + string(e.what()));
    }

    for (auto& atom : atoms) {
        delete atom;
    }
}
