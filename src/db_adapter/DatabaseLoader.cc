#include "DatabaseLoader.h"

#include <filesystem>

#include "ContextLoader.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace std;
using namespace commons;
using namespace atoms;
using namespace db_adapter;

namespace fs = std::filesystem;

// ==============================
//  Construction / destruction
// ==============================

DatabaseMappingOrchestrator::DatabaseMappingOrchestrator(const JsonConfig& config,
                                                         shared_ptr<BoundedSharedQueue> output_queue) {
    this->database_setup(config, output_queue);
    this->tasks = this->strategy->create_tasks(config);
}

DatabaseMappingOrchestrator::~DatabaseMappingOrchestrator() { this->db_conn->stop(); }

// ==============================
//  Public
// ==============================

bool DatabaseMappingOrchestrator::thread_one_step() {
    LOG_DEBUG("DatabaseMappingOrchestrator thread_one_step called. Current task index: "
              << this->current_task);
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

    LOG_DEBUG("Processing task " << task.task_name);

    this->strategy->execute_task(task);

    this->current_task++;
    this->finished = (this->current_task >= this->tasks.size());
    return !this->finished;
}

bool DatabaseMappingOrchestrator::is_finished() const { return this->finished; }

// ==============================
//  Private
// ==============================

void DatabaseMappingOrchestrator::database_setup(const JsonConfig& config,
                                                 shared_ptr<BoundedSharedQueue> output_queue) {
    string adapterdb_type = config.at_path("adapterdb.type").get_or<string>("");

    // TODO: In the future we can implement a factory pattern here to support more database types
    if (adapterdb_type == "postgres") {
        string host = config.at_path("adapterdb.database_credentials.host").get<string>();
        uint port = config.at_path("adapterdb.database_credentials.port").get<uint>();
        string username = config.at_path("adapterdb.database_credentials.username").get<string>();
        string password = config.at_path("adapterdb.database_credentials.password").get<string>();
        string database = config.at_path("adapterdb.database_credentials.database").get<string>();
        auto conn = make_unique<PostgresDatabaseConnection>(
            "psql-conn", host, port, database, username, password);
        auto postgres_wrapper = make_unique<PostgresWrapper>(*conn, output_queue);
        auto postgres_strategy = make_unique<PostgresMappingStrategy>(move(postgres_wrapper));
        this->db_conn = move(conn);
        this->strategy = move(postgres_strategy);
    } else {
        RAISE_ERROR("Unsupported adapter type: " + adapterdb_type);
    }
}

PostgresMappingStrategy::PostgresMappingStrategy(unique_ptr<PostgresWrapper> wrapper)
    : wrapper(move(wrapper)) {}

vector<MappingTask> PostgresMappingStrategy::create_tasks(const JsonConfig& config) {
    vector<string> file_paths =
        config.at_path("adapterdb.context_mapping_paths").get_or<vector<string>>({});

    vector<MappingTask> tasks;

    if (file_paths.empty()) {
        LOG_INFO(
            "No context mapping files specified in config at adapterdb.context_mapping_paths. The "
            "entire database will be mapped.");
        tasks.push_back(MappingTask{"map_all_database", nullopt});
    } else {
        for (const auto& path : file_paths) {
            fs::path p(path);
            string ext = p.extension().string();

            if (ext != ".sql") {
                RAISE_ERROR("Unsupported mapping file type: " + ext + " for file: " + path);
            }

            auto queries_sql = ContextLoader::load_query_file(path);

            if (!queries_sql.empty()) {
                for (size_t i = 0; i < queries_sql.size(); i++) {
                    LOG_DEBUG("Query " << (i + 1) << ": " << queries_sql[i]);
                    tasks.push_back(MappingTask{"custom_query_" + to_string(i), queries_sql[i]});
                }
            }

            LOG_DEBUG(to_string(queries_sql.size()) + " queries were loaded from the query file.");
        }
    }
    return tasks;
}

void PostgresMappingStrategy::execute_task(const MappingTask& task) {
    if (task.context == nullopt) {
        auto tables = this->wrapper->list_tables();

        if (tables.empty()) {
            RAISE_ERROR("No tables found in the database.");
        }

        for (const auto& table : tables) {
            this->wrapper->map_table(table, vector<string>{}, vector<string>{}, false);
        }
    } else {
        this->wrapper->map_sql_query(task.task_name, task.context.value());
    }
}

/**
 * MultiThreadAtomPersister implementation using ThreadPool
 */

// ==============================
//  Construction / destruction
// ==============================

MultiThreadAtomPersister::MultiThreadAtomPersister(shared_ptr<BoundedSharedQueue> input_queue,
                                                   ThreadPool& pool,
                                                   shared_ptr<AtomDB> atomdb,
                                                   size_t batch_size,
                                                   bool save_metta_expression,
                                                   string metta_output_dir,
                                                   size_t max_pending_batches)
    : input_queue(input_queue),
      pool(pool),
      atomdb(atomdb),
      batch_size(batch_size),
      save_metta_expression(save_metta_expression),
      max_pending_batches(max_pending_batches) {
    this->accumulator.reserve(batch_size);

    if (save_metta_expression) {
        this->metta_writer = make_shared<MettaFileWriter>(metta_output_dir);
    }

    LOG_DEBUG("MultiThreadAtomPersister initialized | batch_size: "
              << batch_size << " | max_pending_batches: " << max_pending_batches
              << " | pool: " << this->pool.to_string());
}

MultiThreadAtomPersister::~MultiThreadAtomPersister() {
    if (is_save_metta()) {
        this->metta_writer->close();
    }

    LOG_DEBUG("MultiThreadAtomPersister destroyed | total_atoms: "
              << this->total_count.load() << " | batches_dispatched: " << this->batches_dispatched.load()
              << " | batches_completed: " << this->batches_completed.load()
              << " | batches_failed: " << this->batches_failed.load());
}

// ==============================
//  Public
// ==============================

void MultiThreadAtomPersister::dispatch() {
    if (static_cast<size_t>(this->pool.size()) >= this->max_pending_batches) {
        this->flush_batch();
        return;
    }

    this->drain_into_accumulator();

    this->flush_batch();

    if (this->producer_finished.load() && !this->accumulator.empty()) {
        int batch_id = this->batches_dispatched.fetch_add(1) + 1;

        LOG_DEBUG("Dispatching FINAL batch #" << batch_id << " | size: " << this->accumulator.size()
                                              << " | (remainder < batch_size)");

        vector<Atom*> final_batch = move(this->accumulator);
        this->accumulator.clear();

        shared_ptr<MettaFileWriter> writer = this->metta_writer;

        this->pool.enqueue([this, b = move(final_batch), batch_id, writer]() mutable {
            send_batch(move(b), batch_id, writer);
        });
    }
}

void MultiThreadAtomPersister::set_producer_finished() {
    this->producer_finished.store(true);
    LOG_DEBUG("Producer finished signal received"
              << " | accumulator_size: " << this->accumulator.size() << " | queue_remaining: "
              << this->input_queue->size() << " | total_atoms_so_far: " << this->total_count.load()
              << " | batches_dispatched: " << this->batches_dispatched.load()
              << " | batches_completed: " << this->batches_completed.load());
}

bool MultiThreadAtomPersister::is_producer_finished() const { return this->producer_finished.load(); }

int MultiThreadAtomPersister::get_total_count() const { return this->total_count.load(); }

// ==============================
//  Private
// ==============================

void MultiThreadAtomPersister::drain_into_accumulator() {
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

void MultiThreadAtomPersister::flush_batch() {
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

        shared_ptr<MettaFileWriter> writer = this->metta_writer;
        this->pool.enqueue([this, b = move(batch), batch_id, writer]() mutable {
            send_batch(move(b), batch_id, writer);
        });
    }
}

void MultiThreadAtomPersister::send_batch(vector<Atom*> atoms,
                                          int batch_id,
                                          shared_ptr<MettaFileWriter> writer) {
    StopWatch timer_success;
    StopWatch timer_failure;
    timer_success.start();
    timer_failure.start();

    LOG_DEBUG("Batch #" << batch_id << " started | size: " << atoms.size()
                        << " | thread: " << this_thread::get_id());

    try {
        this->atomdb->add_atoms(atoms, false, true);

        if (this->is_save_metta()) {
            for (auto& atom : atoms) {
                if (atom->arity() > 0) {
                    Link* link = dynamic_cast<Link*>(atom);
                    const string metta_expression =
                        link->custom_attributes.get_or<string>("metta_expression", "");

                    if (metta_expression.empty()) continue;

                    LOG_DEBUG("Thread_ID: " << this_thread::get_id()
                                            << " |  Saving Metta expression: " << metta_expression);
                    writer->write(metta_expression);
                }
            }
        }

        timer_success.stop();

        int new_total =
            this->total_count.fetch_add(static_cast<int>(atoms.size())) + static_cast<int>(atoms.size());
        this->batches_completed.fetch_add(1);

        LOG_DEBUG("Batch #" << batch_id << " completed | size: " << atoms.size()
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

        RAISE_ERROR("Error in batch #" + to_string(batch_id) + ": " + string(e.what()));
    }

    for (auto& atom : atoms) {
        delete atom;
    }
}
