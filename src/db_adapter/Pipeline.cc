#ifndef LOG_LEVEL
#define LOG_LEVEL INFO_LEVEL
#endif
#include "Logger.h"

#include "Pipeline.h"

#include "AtomDBSingleton.h"
#include "DedicatedThread.h"
#include "PostgresWrapper.h"
#include "Processor.h"
#include "SharedQueue.h"

#define BATCH_SIZE 1000

using namespace atomdb;
using namespace std;
using namespace commons;
using namespace atoms;
using namespace db_adapter;

ProducerJob::ProducerJob(shared_ptr<PostgresWrapper> wrapper) : wrapper(wrapper) {}

void ProducerJob::add_task_query(const string& virtual_name, const string& query) {
    this->tasks.push_back(MappingTask{MappingTask::QUERY, TableMapping{}, virtual_name, query});
}

void ProducerJob::add_task_table(TableMapping table_mapping) {
    this->tasks.push_back(MappingTask{MappingTask::TABLE, move(table_mapping), "", ""});
}

bool ProducerJob::thread_one_step() {
    LOG_INFO("ProducerJob thread_one_step called. Current task index: " << this->current_task);
    if (this->current_task >= this->tasks.size()) {
        return false;
    }

    auto& task = this->tasks[this->current_task];

    LOG_INFO("Processing task " << this->current_task << " of type " << (task.type == MappingTask::TABLE ? "TABLE" : "QUERY"));

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

AtomDBJob::AtomDBJob(shared_ptr<SharedQueue> input_queue) : input_queue(input_queue) {
    this->atomdb = AtomDBSingleton::get_instance();
}

bool AtomDBJob::thread_one_step() {
    if (this->input_queue->size() == 0) {
        Utils::sleep();
        return true;
    }
    try {
        vector<Atom*> atoms;
        for (size_t i = 0; i < BATCH_SIZE; ++i) {
            if (this->input_queue->empty()) break;

            auto atom = (Atom*) this->input_queue->dequeue();

            if (atom == nullptr) {
                Utils::error("Dequeued atom is nullptr");
            }
            atoms.push_back(atom);
        }
        if (!atoms.empty()) {
            cout << "Worker processing batch of " << atoms.size() << " atoms." << endl;
            this->atomdb->add_atoms(atoms, false, true);
            for (auto& atom : atoms) {
                delete atom;
            }
        }
        return true;
    } catch (const exception& e) {
        Utils::error("Error in Worker: " + string(e.what()));
    }
    return false;
}

// ==============================================================================

// PipelineProcessor::PipelineProcessor(const string& id, shared_ptr<SharedQueue> input_queue)
//     : Processor(id),
//       input_queue(input_queue),
//       thread_method(make_unique<PipelineThreadMethod>(this)),
//       thread(make_shared<DedicatedThread>(id + "-thread", thread_method.get())) {
//     add_subprocessor(thread);
// }

// void PipelineProcessor::setup() {
//     on_setup();
//     Processor::setup();
// }

// void PipelineProcessor::start() {
//     on_start();
//     Processor::start();
// }

// void PipelineProcessor::stop() {
//     Processor::stop();
//     on_stop();
// }

// PostgresProducerProcessor::PostgresProducerProcessor(const string& id,
//                                                      shared_ptr<PostgresWrapper> db,
//                                                      shared_ptr<SharedQueue> output_queue) {}

// void PostgresProducerProcessor::add_task_query(const string& virtual_name, const string& query) {
//     this->tasks.push_back(MappingTask{MappingTask::QUERY, TableMapping{}, virtual_name, query});
// }

// void PostgresProducerProcessor::add_task_table(TableMapping table_mapping) {
//     this->tasks.push_back(MappingTask{MappingTask::TABLE, move(table_mapping), "", ""});
// }

// void PostgresProducerProcessor::on_setup() {}

// void PostgresProducerProcessor::on_start() {}

// bool PostgresProducerProcessor::produce_one() { return false; }

// void PostgresProducerProcessor::on_stop() {}

// Producer::Producer(const string& id, ThreadMethod* job, shared_ptr<PostgresDatabaseConnection> db_conn, shared_ptr<SharedQueue> queue) {
//     shared_ptr<DedicatedThread> dedicated_thread = make_shared<DedicatedThread>(id, &job);
//     Processor::bind_subprocessor(dedicated_thread, db_conn);
// }

// void Producer::setup() {
//     DedicatedThread::setup();
//     PostgresDBConnection::setup();
// }

// void Producer::start() {
//     PostgresDBConnection::start();
//     DedicatedThread::start();
// }

// void Producer::stop() {
//     DedicatedThread::stop();
//     PostgresDBConnection::stop();
// }
