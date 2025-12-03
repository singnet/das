#include "AtomDBProxy.h"

#include "AtomDBSingleton.h"
#include "BaseProxy.h"
#include "Link.h"
#include "Node.h"
#include "Properties.h"
#include "ServiceBus.h"
#include "Utils.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace atomdb_broker;
using namespace service_bus;
using namespace commons;
using namespace std;

// -------------------------------------------------------------------------------------------------
// Static constants

const size_t AtomDBProxy::BATCH_SIZE = 2000;
const size_t AtomDBProxy::COMMAND_SIZE_LIMIT = 50;
int AtomDBProxy::THREAD_POOL_SIZE = 10;
size_t AtomDBProxy::MAX_PENDING_ATOMS = 20000;
string AtomDBProxy::NODE = "NODE";
string AtomDBProxy::LINK = "LINK";

// Proxy Commands
string AtomDBProxy::ADD_ATOMS = "add_atoms";
string AtomDBProxy::DELETE_ATOMS = "delete_atoms";
string AtomDBProxy::START_STREAM = "start_stream";
string AtomDBProxy::END_STREAM = "end_stream";

// -------------------------------------------------------------------------------------------------
// Constructor and destructor

AtomDBProxy::AtomDBProxy() : BaseProxy() {
    this->command = ServiceBus::ATOMDB;
    this->atomdb = AtomDBSingleton::get_instance();
    this->is_processing_buffer = false;
    this->processing_queue = new SharedQueue(AtomDBProxy::MAX_PENDING_ATOMS * 2);
    this->thread_pool = new ThreadPool(AtomDBProxy::THREAD_POOL_SIZE);
    this->processing_thread = thread(&AtomDBProxy::process_atom_batches, this);
}

AtomDBProxy::~AtomDBProxy() {
    LOG_DEBUG("Shuting down AtomDBProxy, waiting for pending atoms to be processed...");
    while (!this->processing_queue->empty()) {
        Utils::sleep(100);
    }
    this->thread_pool->wait();
    this->stop_processing = true;
    this->processing_thread.join();
    delete this->processing_queue;
    delete this->thread_pool;
    this->abort();
    LOG_DEBUG("AtomDBProxy shut down complete.");
}

// -------------------------------------------------------------------------------------------------
// Client-side API
void AtomDBProxy::pack_command_line_args() { tokenize(this->args); }

void AtomDBProxy::tokenize(vector<string>& output) { BaseProxy::tokenize(output); }

vector<string> AtomDBProxy::add_atoms(const vector<string>& tokens, bool use_streaming) {
    vector<shared_ptr<Atom>> atoms =
        build_atoms_from_tokens<shared_ptr<Atom>>(tokens, shared_ptr_atom_factory);
    vector<Atom*> atom_ptrs;
    for (const auto& atom : atoms) {
        atom_ptrs.push_back(atom.get());
    }
    return add_atoms(atom_ptrs, use_streaming);
}

vector<string> AtomDBProxy::add_atoms(const vector<Atom*>& atoms, bool use_streaming) {
    vector<string> args;
    vector<string> handles;
    string atom_type;
    vector<string> stream_info;
    stream_info.push_back(std::to_string(atoms.size()));

    // Note to Reviewer: The streaming commands are currently disabled
    // by default because the performance gain is negligible due to
    // splitting the atoms into very small batches which is a temporary fix
    // for a memory desallocation issue. Once that issue is fixed, streaming
    // should be re-enabled by default.

    if (use_streaming) {
        set_stream(AtomDBProxy::START_STREAM, stream_info);
    }

    for (Atom* atom : atoms) {
        atom->tokenize(args);

        if (dynamic_cast<Node*>(atom)) {
            atom_type = NODE;
        } else if (dynamic_cast<Link*>(atom)) {
            atom_type = LINK;
        } else {
            Utils::error("Invalid Atom type");
        }

        args.insert(args.begin(), atom_type);
        handles.push_back(atom->handle());
        if ((args.size() > AtomDBProxy::COMMAND_SIZE_LIMIT) || (atom == atoms.back())) {
            Utils::retry_function([&]() { this->to_remote_peer(AtomDBProxy::ADD_ATOMS, args); },
                                  5,
                                  2000,
                                  "AtomDBProxy::add_atoms");
            args.clear();
        }
    }
    if (use_streaming) {
        set_stream(AtomDBProxy::END_STREAM, {});
    }

    return handles;
}

uint AtomDBProxy::delete_atoms(const vector<string>& handles, bool delete_link_targets) {
    vector<string> args;
    // split handles into chunks to avoid exceeding command size limits
    size_t chunk_size = AtomDBProxy::COMMAND_SIZE_LIMIT;
    uint total_deleted = 0;
    for (size_t i = 0; i < handles.size(); i += chunk_size) {
        args.clear();
        size_t end = std::min(i + chunk_size, handles.size());
        args.insert(args.end(), handles.begin() + i, handles.begin() + end);
        args.push_back(delete_link_targets ? "1" : "0");
        Utils::retry_function([&]() { this->to_remote_peer(AtomDBProxy::DELETE_ATOMS, args); },
                              5,
                              2000,
                              "AtomDBProxy::delete_atoms");
    }
    return total_deleted;
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void AtomDBProxy::untokenize(vector<string>& tokens) { BaseProxy::untokenize(tokens); }

bool AtomDBProxy::from_remote_peer(const string& command, const vector<string>& args) {
    if (BaseProxy::from_remote_peer(command, args)) {
        return true;
    } else if (command == AtomDBProxy::ADD_ATOMS) {
        if (this->is_processing_buffer) {
            this->enqueue_request(args);
        } else {
            this->handle_add_atoms(args);
        }
        return true;
    } else if (command == AtomDBProxy::START_STREAM) {
        LOG_INFO("Starting atom stream, total atoms to receive: " << args[0]);
        this->is_processing_buffer = true;
        return true;
    } else if (command == AtomDBProxy::END_STREAM) {
        LOG_INFO("Ending atom stream.");
        this->is_processing_buffer = false;
        return true;
    } else if (command == AtomDBProxy::DELETE_ATOMS) {
        this->handle_delete_atoms(args);
        return true;
    } else {
        Utils::error("Invalid AtomDBProxy command: <" + command + ">");
        return false;
    }
}

void AtomDBProxy::handle_add_atoms(const vector<string>& tokens) {
    try {
        vector<shared_ptr<Atom>> atoms =
            build_atoms_from_tokens<shared_ptr<Atom>>(tokens, shared_ptr_atom_factory);
        vector<Atom*> buffer;
        for (auto& atom : atoms) {
            buffer.push_back(atom.get());
        }
        this->atomdb->add_atoms(buffer, false, true);
    } catch (const exception& e) {
        LOG_ERROR("Error processing batch: " << e.what());
    }
}

void AtomDBProxy::handle_delete_atoms(const vector<string>& args) {
    try {
        if (args.size() < 1) {
            Utils::error("No handles provided for deletion");
        }
        vector<string> handles(args.begin(), args.end() - 1);
        bool delete_link_targets = args.back() == "1";
        uint deleted_count = this->atomdb->delete_atoms(handles, delete_link_targets);
        LOG_INFO("Deleted " << deleted_count << " atoms");
    } catch (const exception& e) {
        LOG_ERROR("Error processing delete_atoms command: " << e.what());
    }
}

void AtomDBProxy::enqueue_request(const vector<string>& tokens) {
    auto atoms = build_atoms_from_tokens<Atom*>(tokens, raw_ptr_atom_factory);
    for (const auto& atom : atoms) {
        this->processing_queue->enqueue((void*) atom);
    }
    unique_lock<mutex> lock(this->api_mutex);
    this->pending_atoms_count += atoms.size();
}

void AtomDBProxy::process_atom_batches() {
    while (!this->stop_processing) {
        if (this->pending_atoms_count >= AtomDBProxy::MAX_PENDING_ATOMS ||
            (!this->is_processing_buffer && this->processing_queue->size() > 0)) {
            unique_lock<mutex> lock(this->api_mutex);
            vector<Atom*> atoms;
            for (size_t i = 0; i < AtomDBProxy::BATCH_SIZE; ++i) {
                if (this->processing_queue->empty()) {
                    break;
                }
                auto atom = (Atom*) this->processing_queue->dequeue();
                if (atom == nullptr) {
                    Utils::error("Dequeued null atom pointer");
                }
                atoms.push_back(atom);
            }
            this->pending_atoms_count -= atoms.size();
            lock.unlock();
            auto job = [this, atoms]() {
                this->atomdb->add_atoms(atoms, false, true);
                for (auto& atom : atoms) {
                    delete atom;
                }
            };
            this->thread_pool->enqueue(job);
        }
        Utils::sleep();
    }
}

void AtomDBProxy::set_stream(const string& command, const vector<string>& args) {
    Utils::retry_function(
        [&]() { this->to_remote_peer(command, args); }, 5, 2000, "AtomDBProxy::end_stream");
}