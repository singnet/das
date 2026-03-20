#include "Pipeline.h"

#include "AtomDBSingleton.h"
#include "DedicatedThread.h"
#include "Logger.h"
#include "PostgresWrapper.h"
#include "Processor.h"
#include "SharedQueue.h"
#include "Utils.h"

#include <queue>

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace std;
using namespace commons;
using namespace atoms;
using namespace db_adapter;

namespace {

// PostgresWrapper enqueues heap-allocated std::queue<Atom*>* per row; drain and delete the wrapper.
void drain_atom_batch_queue_to_vector(vector<Atom*>& out, void* raw_ptr) {
    if (raw_ptr == nullptr) {
        return;
    }
    auto* batch_queue = static_cast<queue<Atom*>*>(raw_ptr);
    while (!batch_queue->empty()) {
        Atom* atom = batch_queue->front();
        batch_queue->pop();
        if (atom != nullptr) {
            out.push_back(atom);
        }
    }
    delete batch_queue;
}

}  // namespace

DatabaseMappingJob::DatabaseMappingJob(const string& host,
                                       int port,
                                       const string& database,
                                       const string& user,
                                       const string& password,
                                       MAPPER_TYPE mapper_type,
                                       shared_ptr<SharedQueue> output_queue) {
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

AtomPersistenceJob::AtomPersistenceJob(shared_ptr<SharedQueue> input_queue) : input_queue(input_queue) {
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
        while (this->atoms.size() < BATCH_SIZE && !this->input_queue->empty()) {
            void* raw = this->input_queue->dequeue();
            size_t before = this->atoms.size();
            drain_atom_batch_queue_to_vector(this->atoms, raw);
            this->count += static_cast<int>(this->atoms.size() - before);
        }
        if (!this->atoms.empty() && this->atoms.size() >= BATCH_SIZE) {
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
        for (auto& atom : this->atoms) {
            delete atom;
        }
        this->atoms.clear();
        Utils::error("Error in Worker: " + string(e.what()));
    }

    return false;
}

bool AtomPersistenceJob::is_finished() const { return this->finished; }

void AtomPersistenceJob::set_producer_finished() { this->producer_finished = true; }

/*
    AtomPersistenceJob2 implementation using ThreadPool
*/

AtomPersistenceJob2::AtomPersistenceJob2(shared_ptr<SharedQueue> input_queue)
    : input_queue(input_queue) {
    this->atomdb = AtomDBSingleton::get_instance();
}

AtomPersistenceJob2::~AtomPersistenceJob2() {}

static double get_available_ram_ratio() {
    unsigned long total = Utils::get_total_ram();
    if (total == 0) return 0.0;
    return static_cast<double>(Utils::get_current_free_ram()) / static_cast<double>(total);
}

void AtomPersistenceJob2::consumer_task() {
    LOG_INFO("== CONSUMER WORKER STARTED ==");

    vector<Atom*> local_atoms;
    // Reserve to avoid repeated reallocations while draining row batches into one AtomDB batch.
    local_atoms.reserve(BATCH_SIZE);

    while (true) {
        while (get_available_ram_ratio() < 0.20) {
            LOG_INFO("Consumer: low RAM, waiting before accumulating more atoms...");
            Utils::sleep(2000);
        }

        while (local_atoms.size() < BATCH_SIZE) {
            void* raw_ptr = this->input_queue->blocking_dequeue(200);
            if (raw_ptr == nullptr) break;

            drain_atom_batch_queue_to_vector(local_atoms, raw_ptr);
        }

        if (!local_atoms.empty() &&
            (local_atoms.size() >= BATCH_SIZE || this->producer_finished.load())) {
            LOG_INFO("Thread processing batch of " << local_atoms.size() << " atoms.");
            try {
                this->atomdb->add_atoms(local_atoms, false, true);
                this->total_count += local_atoms.size();
                for (auto& atom : local_atoms) {
                    delete atom;
                }
                local_atoms.clear();
            } catch (const exception& e) {
                for (auto& atom : local_atoms) {
                    delete atom;
                }
                local_atoms.clear();
                Utils::error("Error in Worker: " + string(e.what()));
            }
        }

        if (this->producer_finished.load() && this->input_queue->empty() && local_atoms.empty()) {
            LOG_INFO("Consumer task finishing. Thread exiting.");
            break;
        }
    }
}

void AtomPersistenceJob2::set_producer_finished() { this->producer_finished.store(true); }