#include "AtomDBProxy.h"

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

// Proxy Commands
string AtomDBProxy::ADD_ATOMS = "add_atoms";

// -------------------------------------------------------------------------------------------------
// Constructor and destructor

AtomDBProxy::AtomDBProxy() : BaseProxy(), m_shutdown(false) {
    this->command = ServiceBus::ATOMDB;
    this->atomdb = AtomDBSingleton::get_instance();

    // Start thread that will consume the queue
    m_worker_thread = thread(&AtomDBProxy::worker_loop, this);
    const char* thread_name = "AtomDBWorker";
    pthread_setname_np(m_worker_thread.native_handle(), thread_name);

}

AtomDBProxy::~AtomDBProxy() {
    LOG_INFO("Shutting down AtomDBProxy worker thread");
    m_shutdown = true;
    m_queue_cond.notify_one();
    if (m_worker_thread.joinable()) {
        m_worker_thread.join();
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

        if (dynamic_cast<Node*>(atom)) {
            atom_type = "NODE";
        } else if (dynamic_cast<Link*>(atom)) {
            atom_type = "LINK";
        } else {
            Utils::error("Invalid Atom type");
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
    } else {
        Utils::error("Invalid AtomDBProxy command: <" + command + ">");
        return false;
    }
}

void AtomDBProxy::handle_add_atoms(const vector<string>& tokens) {
    // {
    //     lock_guard<mutex> lock(m_queue_mutex);
    //     m_work_queue.push(tokens);
    // }
    // m_queue_cond.notify_one();

    try {
        vector<Atom*> atoms = build_atoms_from_tokens(tokens);
        LOG_INFO("Processing batch, total atoms to process: " << atoms.size());

        if (atoms.empty()) {
            LOG_INFO("No atoms were built from tokens. Nothing to process.");
            return;
        }

        const size_t BATCH_SIZE = 1000;

        for (size_t i = 0; i < atoms.size(); i += BATCH_SIZE) {
            auto batch_start = atoms.begin() + i;
            auto batch_end = atoms.begin() + min(i + BATCH_SIZE, atoms.size());
            vector<Atom*> buffer(batch_start, batch_end);
            LOG_INFO("Processing a batch of " << buffer.size() << " atoms.");
            this->atomdb->add_atoms(buffer);
            Utils::sleep(1000);
        }

        LOG_INFO("Finished processing all batches.");

    } catch (const exception& e) {
        LOG_ERROR("Error processing batch: " << e.what());
    }
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

void AtomDBProxy::worker_loop() {
    while (!m_shutdown) {
        vector<string> tokens;

        {
            unique_lock<mutex> lock(m_queue_mutex);
            m_queue_cond.wait(lock, [this] {
                return !m_work_queue.empty() || m_shutdown;
            });

            if (m_shutdown) break;

            tokens = m_work_queue.front();
            m_work_queue.pop();
        }

    try {
        vector<Atom*> atoms = build_atoms_from_tokens(tokens);
        LOG_INFO("Processing batch from worker thread, total atoms to process: " << atoms.size());

        if (atoms.empty()) {
            LOG_INFO("No atoms were built from tokens. Nothing to process.");
            return;
        }

        const size_t BATCH_SIZE = 2000;

        for (size_t i = 0; i < atoms.size(); i += BATCH_SIZE) {
            auto batch_start = atoms.begin() + i;
            auto batch_end = atoms.begin() + min(i + BATCH_SIZE, atoms.size());
            vector<Atom*> buffer(batch_start, batch_end);
            LOG_INFO("Processing a batch of " << buffer.size() << " atoms.");
            this->atomdb->add_atoms(buffer);
            Utils::sleep(5000);
        }

        LOG_INFO("Finished processing all batches.");

    } catch (const exception& e) {
        LOG_ERROR("Error processing batch from worker thread: " << e.what());
    }
    }
}
