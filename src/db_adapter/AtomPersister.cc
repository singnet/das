#include "AtomPersister.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace std;
using namespace commons;
using namespace atoms;
using namespace db_adapter;

// ==============================
//  Construction / destruction
// ==============================

AtomPersister::AtomPersister(shared_ptr<BoundedSharedQueue> input_queue,
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

    LOG_DEBUG("AtomPersister initialized | batch_size: " << batch_size << " | max_pending_batches: "
                                                         << max_pending_batches
                                                         << " | pool: " << this->pool.to_string());
}

AtomPersister::~AtomPersister() {
    if (is_save_metta()) {
        this->metta_writer->close();
    }

    LOG_DEBUG("AtomPersister destroyed | total_atoms: "
              << this->total_count.load() << " | batches_dispatched: " << this->batches_dispatched.load()
              << " | batches_completed: " << this->batches_completed.load()
              << " | batches_failed: " << this->batches_failed.load());
}

// ==============================
//  Public
// ==============================

void AtomPersister::dispatch() {
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

void AtomPersister::set_producer_finished() {
    this->producer_finished.store(true);
    LOG_DEBUG("Producer finished signal received"
              << " | accumulator_size: " << this->accumulator.size() << " | queue_remaining: "
              << this->input_queue->size() << " | total_atoms_so_far: " << this->total_count.load()
              << " | batches_dispatched: " << this->batches_dispatched.load()
              << " | batches_completed: " << this->batches_completed.load());
}

bool AtomPersister::is_producer_finished() const { return this->producer_finished.load(); }

int AtomPersister::get_total_count() const { return this->total_count.load(); }

// ==============================
//  Private
// ==============================

void AtomPersister::drain_into_accumulator() {
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

void AtomPersister::flush_batch() {
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

void AtomPersister::send_batch(vector<Atom*> atoms, int batch_id, shared_ptr<MettaFileWriter> writer) {
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
#if LOG_LEVEL >= DEBUG_LEVEL
        int new_total =
            this->total_count.fetch_add(static_cast<int>(atoms.size())) + static_cast<int>(atoms.size());
#endif
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
