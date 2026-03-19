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
    // this->set_pattern_index_schema();
}

void AtomPersistenceJob::set_pattern_index_schema() {
    auto db = dynamic_pointer_cast<RedisMongoDB>(AtomDBSingleton::get_instance());

    // 1. (Evaluation Predicate Concept)
    string tokens = "LINK_TEMPLATE Expression 3 NODE Symbol Evaluation VARIABLE v1 VARIABLE v2";
    vector<vector<string>> index_entries = {{"_", "*", "*"}, {"_", "v1", "*"}, {"_", "*", "v2"}};
    db->add_pattern_index_schema(tokens, index_entries);

    // 2. (Predicate X) e (Concept X)
    tokens = "LINK_TEMPLATE Expression 2 NODE Symbol Predicate VARIABLE v1";
    index_entries = {{"_", "*"}, {"_", "v1"}};
    db->add_pattern_index_schema(tokens, index_entries);

    tokens = "LINK_TEMPLATE Expression 2 NODE Symbol Concept VARIABLE v1";
    index_entries = {{"_", "*"}, {"_", "v1"}};
    db->add_pattern_index_schema(tokens, index_entries);

    // 3. (table_name value) or (column_name link_fk)
    tokens = "LINK_TEMPLATE Expression 2 VARIABLE v1 VARIABLE v2";
    index_entries = {{"v1", "*"}, {"*", "v2"}};
    db->add_pattern_index_schema(tokens, index_entries);

    // 4. (table_name column_name value)
    tokens = "LINK_TEMPLATE Expression 3 VARIABLE v1 VARIABLE v2 VARIABLE v3";
    index_entries = {{"v1", "v2", "*"}, {"v1", "*", "v3"}, {"*", "v2", "v3"}};
    db->add_pattern_index_schema(tokens, index_entries);
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
            if (atom == nullptr) {
                Utils::error("Dequeued atom is nullptr");
            }
            if (!this->has_handle(atom->handle())) {
                this->atoms.push_back(atom);
            } else {
                delete atom;
            }
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

bool AtomPersistenceJob::has_handle(const string& handle) {
    auto result = this->handle_set.insert(handle);
    return !result.second;
}

/*
    AtomPersistenceJob2 implementation using ThreadPool
*/

AtomPersistenceJob2::AtomPersistenceJob2(shared_ptr<SharedQueue> input_queue)
    : input_queue(input_queue) {
    this->atomdb = AtomDBSingleton::get_instance();
}

AtomPersistenceJob2::~AtomPersistenceJob2() {}

bool AtomPersistenceJob2::has_handle(const string& handle) {
    auto result = this->handle_set.insert(handle);
    return !result.second;
}

void AtomPersistenceJob2::consumer_task() {
    LOG_INFO("== CONSUMER WORKER STARTED ==");

    vector<Atom*> local_atoms;

    while (true) {
        while (local_atoms.size() < BATCH_SIZE) {
            if (this->input_queue->empty()) break;

            void* raw_ptr = this->input_queue->dequeue();
            std::queue<Atom*>* batch_queue = static_cast<std::queue<Atom*>*>(raw_ptr);

            if (batch_queue != nullptr) {
                while (!batch_queue->empty()) {
                    Atom* atom = batch_queue->front();
                    batch_queue->pop();

                    if (atom != nullptr) {
                        if (!this->has_handle(atom->handle())) {
                            local_atoms.push_back(atom);
                        } else {
                            delete atom;
                        }
                    }
                }
                delete batch_queue;
            }
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
                Utils::error("Error in Worker: " + string(e.what()));
                for (auto& atom : local_atoms) {
                    delete atom;
                }
                local_atoms.clear();
            }
        }

        Utils::sleep();

        if (this->producer_finished.load() && this->input_queue->empty() && local_atoms.empty()) {
            LOG_INFO("Consumer task finishing. Thread exiting.");
            break;
        }

        if (this->input_queue->empty() && !this->producer_finished.load()) {
            Utils::sleep();
        }
    }
}

void AtomPersistenceJob2::set_producer_finished() { this->producer_finished.store(true); }