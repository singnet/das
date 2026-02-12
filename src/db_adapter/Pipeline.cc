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

PostgresWrapperJob::PostgresWrapperJob(shared_ptr<PostgresWrapper> db) : db(db) {}

void PostgresWrapperJob::add_task_query(const string& virtual_name, const string& query) {
    this->tasks.push_back(MappingTask{MappingTask::QUERY, TableMapping{}, virtual_name, query});
}
void PostgresWrapperJob::add_task_table(TableMapping table_mapping) {
    this->tasks.push_back(MappingTask{MappingTask::TABLE, move(table_mapping), "", ""});
}

bool PostgresWrapperJob::thread_one_step() {
    if (this->current_task >= this->tasks.size()) {
        return false;
    }

    auto& task = this->tasks[this->current_task];

    if (task.type == MappingTask::TABLE) {
        auto table = this->db->get_table(task.table_mapping.table_name);
        this->db->map_table(table,
                            task.table_mapping.where_clauses.value_or(vector<string>{}),
                            task.table_mapping.skip_columns.value_or(vector<string>{}),
                            false);
    } else if (task.type == MappingTask::QUERY) {
        this->db->map_sql_query(task.virtual_name, task.query);
    }

    this->current_task++;
    return (this->current_task < this->tasks.size());
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

FileJob::FileJob(shared_ptr<SharedQueue> input_queue, const string& file_path)
    : input_queue(input_queue), file_path(file_path) {
    // Open the file
}

bool FileJob::thread_one_step() {
    if (this->input_queue->size() == 0) {
        Utils::sleep();
        return true;
    }
    try {
        // Send expressions to file
        return true;
    } catch (const exception& e) {
        Utils::error("Error in Worker: " + string(e.what()));
    }
    return false;
}

// ==============================================================================

PipelineProcessor::PipelineProcessor(const string& id, shared_ptr<SharedQueue> input_queue)
    : Processor(id),
      input_queue(input_queue),
      thread_method(make_unique<PipelineThreadMethod>(this)),
      thread(make_shared<DedicatedThread>(id + "-thread", thread_method.get())) {
    add_subprocessor(thread);
}

void PipelineProcessor::setup() {
    on_setup();
    Processor::setup();
}

void PipelineProcessor::start() {
    on_start();
    Processor::start();
}

void PipelineProcessor::stop() {
    Processor::stop();
    on_stop();
}

PostgresProducerProcessor::PostgresProducerProcessor(const string& id,
                                                     shared_ptr<PostgresWrapper> db,
                                                     shared_ptr<SharedQueue> output_queue) {}

void PostgresProducerProcessor::add_task_query(const string& virtual_name, const string& query) {
    this->tasks.push_back(MappingTask{MappingTask::QUERY, TableMapping{}, virtual_name, query});
}

void PostgresProducerProcessor::add_task_table(TableMapping table_mapping) {
    this->tasks.push_back(MappingTask{MappingTask::TABLE, move(table_mapping), "", ""});
}

void PostgresProducerProcessor::on_setup() {}

void PostgresProducerProcessor::on_start() {}

bool PostgresProducerProcessor::produce_one() { return false; }

void PostgresProducerProcessor::on_stop() {}
