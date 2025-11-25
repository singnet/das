#include "AtomDBProxy.h"

#include <malloc.h>

#include <numeric>

#include "AtomDBSingleton.h"
#include "BaseProxy.h"
#include "Link.h"
#include "Node.h"
#include "Properties.h"
#include "ServiceBus.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb_broker;
using namespace service_bus;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Static constants

const size_t AtomDBProxy::BATCH_SIZE = 100000;
const size_t AtomDBProxy::NUM_THREADS = 10;

// Proxy Commands
string AtomDBProxy::ADD_ATOMS = "add_atoms";
string AtomDBProxy::PERSIST_PENDING = "persist_pending";

// -------------------------------------------------------------------------------------------------
// Constructor and destructor

AtomDBProxy::AtomDBProxy() : BaseProxy() {
    this->command = ServiceBus::ATOMDB;
    this->atomdb = AtomDBSingleton::get_instance();

    for (size_t i = 0; i < NUM_THREADS; ++i) {
        this->workers.emplace_back(&AtomDBProxy::worker_loop, this);
    }
}

AtomDBProxy::~AtomDBProxy() {
    LOG_INFO("Shutdown AtomDBProxy...");
    persist_pending();
    {
        unique_lock<mutex> lock(this->queue_mutex);
        this->stop_processing = true;
    }
    this->queue_condition.notify_all();

    for (thread& worker : this->workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    this->abort();
}

// -------------------------------------------------------------------------------------------------
// Client-side API
void AtomDBProxy::pack_command_line_args() { tokenize(this->args); }

void AtomDBProxy::tokenize(vector<string>& output) { BaseProxy::tokenize(output); }

/**
 * Build a single argument vector containing, for each atom, a leading type
 * token ("NODE" or "LINK") followed by that atom's tokenized fields.
 */
vector<string> AtomDBProxy::add_atoms(const vector<Atom*>& atoms) {
    vector<string> args;
    vector<string> handles;
    string atom_type;

    for (Atom* atom : atoms) {
        atom->tokenize(args);

        if (atom->arity() == 0) {
            atom_type = "NODE";
        } else {
            atom_type = "LINK";
        }

        args.insert(args.begin(), atom_type);
        handles.push_back(atom->handle());
    }

    to_remote_peer(AtomDBProxy::ADD_ATOMS, args);

    return handles;
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void AtomDBProxy::untokenize(vector<string>& tokens) { BaseProxy::untokenize(tokens); }

bool AtomDBProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in "
                                 << this->my_id());

    if (BaseProxy::from_remote_peer(command, args)) {
        return true;
    } else if (command == AtomDBProxy::ADD_ATOMS) {
        handle_add_atoms(args);
        return true;
    } else if (command == AtomDBProxy::PERSIST_PENDING) {
        persist_pending();
        return true;
    } else {
        Utils::error("Invalid AtomDBProxy command: <" + command + ">");
        return false;
    }
}

void AtomDBProxy::handle_add_atoms(const vector<string>& tokens) {
    vector<Atom*> atoms = build_atoms_from_tokens(tokens);
    LOG_INFO("Received " << atoms.size() << " atoms from peer " << this->peer_id());
    add_work(move(atoms));
}

void AtomDBProxy::persist_pending() {
    unique_lock<mutex> lock(this->queue_mutex);

    if (this->accumulator.empty()) {
        LOG_INFO("[Persist] Received persist command, but accumulator is empty. Nothing to do.");
        return;
    }

    LOG_INFO("[Persist] Received persist command. Flushing " << this->accumulator.size()
                                                             << " remaining batches from accumulator.");

    this->batches_ready_to_process.push(move(this->accumulator));

    this->accumulator = vector<Atom*>();
    this->accumulator.reserve(BATCH_SIZE);

    this->queue_condition.notify_one();
}

vector<Atom*> AtomDBProxy::build_atoms_from_tokens(const vector<string>& tokens) {
    vector<Atom*> atoms;
    string current;
    vector<string> buffer;

    auto flush = [&]() {
        if (current.empty()) return;
        if (current == "NODE") {
            atoms.push_back(new Node(buffer));
        } else {  // LINK
            atoms.push_back(new Link(buffer));
        }
        buffer.clear();
    };

    for (const auto& token : tokens) {
        if (token == "NODE" || token == "LINK") {
            if (!current.empty()) flush();
            current = token;
        } else {
            buffer.push_back(token);
        }
    }

    if (!buffer.empty() && !current.empty()) flush();

    return atoms;
}

void AtomDBProxy::add_work(vector<Atom*> atoms) {
    if (atoms.empty()) return;

    unique_lock<mutex> lock(this->queue_mutex);

    size_t required_size = this->accumulator.size() + atoms.size();

    if (this->accumulator.capacity() < required_size) {
        this->accumulator.reserve(required_size);
    }

    this->accumulator.insert(
        this->accumulator.end(), make_move_iterator(atoms.begin()), make_move_iterator(atoms.end()));

    if (this->accumulator.size() >= BATCH_SIZE) {
        this->batches_ready_to_process.push(move(this->accumulator));

        this->accumulator = vector<Atom*>();
        this->accumulator.reserve(BATCH_SIZE);

        this->queue_condition.notify_one();
    }
}

void AtomDBProxy::worker_loop() {
    while (true) {
        vector<Atom*> batch;
        {
            unique_lock<mutex> lock(this->queue_mutex);

            this->queue_condition.wait(lock, [this] {
                return this->stop_processing || !this->batches_ready_to_process.empty();
            });

            if (this->stop_processing && this->batches_ready_to_process.empty()) {
                return;
            }

            batch = move(this->batches_ready_to_process.front());
            this->batches_ready_to_process.pop();
        }

        LOG_INFO("[Thread " << this_thread::get_id() << "] Processing batch with " << batch.size()
                            << " atoms.");

        try {
            this->atomdb->add_atoms(batch, false, true);
            LOG_INFO("[Thread " << this_thread::get_id() << "] batch processed successfully.");
        } catch (const exception& e) {
            LOG_ERROR("Error processing batch: " << e.what());
        }

        for (Atom* atom : batch) {
            delete atom;
        }
        batch.clear();

        // malloc_trim(0);
    }
}
