#include "AtomDBBrokerProxy.h"
#include "BaseProxy.h"
#include "ServiceBus.h"
#include "AtomDBSingleton.h"
#include "Link.h"
#include "Node.h"
#include "Utils.h"
#include "Properties.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb_broker;
using namespace service_bus;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Static constants

// Proxy Commands
string AtomDBBrokerProxy::ADD_ATOMS = "add_atoms";
string AtomDBBrokerProxy::SHUTDOWN = "shutdown";

// -------------------------------------------------------------------------------------------------
// Constructor and destructor

AtomDBBrokerProxy::AtomDBBrokerProxy() : BaseProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::ATOMDB;
    this->atomdb = AtomDBSingleton::get_instance();
    this->keep_alive_flag = true;
}

AtomDBBrokerProxy::~AtomDBBrokerProxy() { to_remote_peer(SHUTDOWN, {}); }

// -------------------------------------------------------------------------------------------------
// Client-side API
void AtomDBBrokerProxy::shutdown() { to_remote_peer(SHUTDOWN, {}); }

bool AtomDBBrokerProxy::running() const { return this->keep_alive_flag; }

void AtomDBBrokerProxy::pack_command_line_args() { tokenize(this->args); }

void AtomDBBrokerProxy::tokenize(vector<string>& output) { BaseProxy::tokenize(output); }

/**
 * Build a single argument vector containing, for each atom, a leading type
 * token ("NODE" or "LINK") followed by that atom's tokenized fields.
 */     
vector<string> AtomDBBrokerProxy::add_atoms(const vector<Atom*>& atoms) {
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
    
    to_remote_peer(AtomDBBrokerProxy::ADD_ATOMS, args);

    return handles;
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void AtomDBBrokerProxy::untokenize(vector<string>& tokens) { BaseProxy::untokenize(tokens); }

bool AtomDBBrokerProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in " << this->my_id());
   
    if (BaseProxy::from_remote_peer(command, args)) {
        return true;
    } else if (command == AtomDBBrokerProxy::ADD_ATOMS) {
        handle_add_atoms(args);
        return true;
    } else if (command == AtomDBBrokerProxy::SHUTDOWN) {
        this->keep_alive_flag = false;
        return true;
    } else {
        Utils::error("Invalid AtomDBBrokerProxy command: <" + command + ">");
        return false;
    }
}

void AtomDBBrokerProxy::handle_add_atoms(const vector<string>& tokens) {
    try {
        vector<Atom*> atoms = build_atoms_from_tokens(tokens);
        this->atomdb->add_atoms(atoms);
    } catch (const std::runtime_error& exception) {
        raise_error_on_peer(exception.what());
    } catch (const std::exception& exception) {
        raise_error_on_peer(exception.what());
    }
}

vector<Atom*> AtomDBBrokerProxy::build_atoms_from_tokens(const vector<string>& tokens) {
    vector<Atom*> atoms;
    string current;
    vector<string> buffer;

    auto flush = [&]() {
        if (current.empty()) return;
        if (current == "NODE") {
            atoms.push_back(new Node(buffer));
        } else { // LINK
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
