#include "Pipeline.h"

#include "AtomDBSingleton.h"
#include "DedicatedThread.h"
#include "Logger.h"
#include "PostgresWrapper.h"
#include "Processor.h"
#include "SharedQueue.h"

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
                                       shared_ptr<SharedQueue> output_queue) {
    this->db_conn =
        make_unique<PostgresDatabaseConnection>("psql-conn", host, port, database, user, password);
    this->wrapper = make_unique<PostgresWrapper>(*db_conn, mapper_type, output_queue);
}

DatabaseMappingJob::~DatabaseMappingJob() {
    if (this->db_conn->is_running()) {
        this->db_conn->stop();
    }
}

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
            if (atom == nullptr) {
                Utils::error("Dequeued atom is nullptr");
            }
            this->atoms.push_back(atom);
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
