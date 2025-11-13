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

const size_t AtomDBProxy::BATCH_SIZE = 5000;

// Proxy Commands
string AtomDBProxy::ADD_ATOMS = "add_atoms";

// -------------------------------------------------------------------------------------------------
// Constructor and destructor

AtomDBProxy::AtomDBProxy() : BaseProxy() {
    this->command = ServiceBus::ATOMDB;
    this->atomdb = AtomDBSingleton::get_instance();
}

AtomDBProxy::~AtomDBProxy() {
    LOG_INFO("Shutdown AtomDBProxy...");
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
    try {
        vector<Atom*> atoms = build_atoms_from_tokens(tokens);
        LOG_INFO("Processing " << atoms.size() << " atoms...");

        if (atoms.empty()) {
            LOG_INFO("No atoms were built from tokens. Nothing to process.");
            return;
        }

        this->atomdb->add_atoms(atoms, false, true);

        LOG_INFO("Finished processing all atoms.");

    } catch (const exception& e) {
        LOG_ERROR("Error processing atoms: " << e.what());
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
