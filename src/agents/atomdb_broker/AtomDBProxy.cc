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

const size_t AtomDBProxy::BATCH_SIZE = 1000;
const size_t AtomDBProxy::COMMAND_SIZE_LIMIT = 50;

// Proxy Commands
string AtomDBProxy::ADD_ATOMS = "add_atoms";
string AtomDBProxy::START_STREAM = "start_stream";
string AtomDBProxy::END_STREAM = "end_stream";

// -------------------------------------------------------------------------------------------------
// Constructor and destructor

AtomDBProxy::AtomDBProxy() : BaseProxy() {
    this->command = ServiceBus::ATOMDB;
    this->atomdb = AtomDBSingleton::get_instance();
    this->processing_buffer = false;
}

AtomDBProxy::~AtomDBProxy() {
    LOG_INFO("Shutdown AtomDBProxy...");
    this->abort();
}

// -------------------------------------------------------------------------------------------------
// Client-side API
void AtomDBProxy::pack_command_line_args() { tokenize(this->args); }

void AtomDBProxy::tokenize(vector<string>& output) { BaseProxy::tokenize(output); }

vector<string> AtomDBProxy::add_atoms(const vector<string>& tokens) {
    vector<shared_ptr<Atom>> atoms = build_atoms_from_tokens(tokens);
    vector<Atom*> atom_ptrs;
    for (const auto& atom : atoms) {
        atom_ptrs.push_back(atom.get());
    }
    return add_atoms(atom_ptrs);
}

/**
 * Build a single argument vector containing, for each atom, a leading type
 * token ("NODE" or "LINK") followed by that atom's tokenized fields.
 */
vector<string> AtomDBProxy::add_atoms(const vector<Atom*>& atoms) {
    vector<string> args;
    vector<string> handles;
    string atom_type;
    vector<string> stream_info;
    stream_info.push_back(std::to_string(atoms.size()));

    Utils::retry_function([&]() { this->to_remote_peer(AtomDBProxy::START_STREAM, stream_info); },
                          5,
                          2000,
                          "AtomDBProxy::start_stream");

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
        if ((args.size() > AtomDBProxy::COMMAND_SIZE_LIMIT) || (atom == atoms.back())) {
            Utils::retry_function([&]() { this->to_remote_peer(AtomDBProxy::ADD_ATOMS, args); },
                                  5,
                                  2000,
                                  "AtomDBProxy::add_atoms");
            args.clear();
        }
    }
    Utils::retry_function([&]() { this->to_remote_peer(AtomDBProxy::END_STREAM, stream_info); },
                          5,
                          2000,
                          "AtomDBProxy::end_stream");

    return handles;
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void AtomDBProxy::untokenize(vector<string>& tokens) { BaseProxy::untokenize(tokens); }

bool AtomDBProxy::from_remote_peer(const string& command, const vector<string>& args) {
    if (BaseProxy::from_remote_peer(command, args)) {
        return true;
    } else if (command == AtomDBProxy::ADD_ATOMS) {
        handle_add_atoms(args);
        return true;
    } else if (command == AtomDBProxy::START_STREAM) {
        LOG_INFO("Starting atom stream, total atoms to receive: " << args[0]);
        this->processing_buffer = true;
        return true;
    } else if (command == AtomDBProxy::END_STREAM) {
        LOG_INFO("Ending atom stream.");
        this->processing_buffer = false;
        return true;
    } else {
        Utils::error("Invalid AtomDBProxy command: <" + command + ">");
        return false;
    }
}

void AtomDBProxy::handle_add_atoms(const vector<string>& tokens) {
    try {
        vector<shared_ptr<Atom>> atoms = build_atoms_from_tokens(tokens);
        vector<Atom*> buffer;
        for (auto& atom : atoms) {
            buffer.push_back(atom.get());
        }
        this->atomdb->add_atoms(buffer, false, true);
    } catch (const exception& e) {
        LOG_ERROR("Error processing batch: " << e.what());
    }
}

vector<shared_ptr<Atom>> AtomDBProxy::build_atoms_from_tokens(const vector<string>& tokens) {
    vector<shared_ptr<Atom>> atoms;
    string current;
    vector<string> buffer;

    auto flush = [&]() {
        if (current.empty()) return;
        if (current == "NODE") {
            atoms.push_back(make_shared<Node>(buffer));
        } else {  // LINK
            atoms.push_back(make_shared<Link>(buffer));
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
