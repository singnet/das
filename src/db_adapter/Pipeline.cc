#ifndef LOG_LEVEL
#define LOG_LEVEL INFO_LEVEL
#endif
#include "Pipeline.h"

#include "AtomDBSingleton.h"
#include "DedicatedThread.h"
#include "Logger.h"
#include "PostgresWrapper.h"
#include "Processor.h"
#include "SharedQueue.h"

#define BATCH_SIZE 1000

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
    if (this->input_queue->empty()) {
        Utils::sleep();
        this->finished = false;
        return false;
    }
    vector<Atom*> atoms;
    try {
        for (size_t i = 0; i < BATCH_SIZE; ++i) {
            if (this->input_queue->empty()) break;

            auto atom = (Atom*) this->input_queue->dequeue();

            if (atom == nullptr) {
                Utils::error("Dequeued atom is nullptr");
            }
            atoms.push_back(atom);
        }
        if (!atoms.empty()) {
            LOG_DEBUG("Worker processing batch of " << atoms.size() << " atoms." << endl);
            this->atomdb->add_atoms(atoms, false, true);
            for (auto& atom : atoms) {
                delete atom;
            }
        }
        this->finished = this->input_queue->empty();
        return true;
    } catch (const exception& e) {
        Utils::error("Error in Worker: " + string(e.what()));
        for (auto& atom : atoms) {
            delete atom;
        }
    }
    this->finished = false;
    return false;
}

bool AtomPersistenceJob::is_finished() const { return this->finished; }
