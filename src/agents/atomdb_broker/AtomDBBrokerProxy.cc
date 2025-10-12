#include "AtomDBBrokerProxy.h"
#include "ServiceBus.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb_broker;

// -------------------------------------------------------------------------------------------------
// Static constants

// Proxy Commands
static AtomDBBrokerProxy::ADD_ATOMS = "add_atoms";

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

AtomDBBrokerProxy::AtomDBBrokerProxy() : BaseProxy() {
    // Typically used in processor
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::ATOMDB;
    this->atomdb = AtomDBSingleton::get_instance();
}

AtomDBBrokerProxy::AtomDBBrokerProxy(const string& command, const vector<string>& tokens) : BaseProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::ATOMDB;
    this->tokens = tokens;
}

AtomDBBrokerProxy::~AtomDBBrokerProxy() {}

void AtomDBBrokerProxy::pack_command_line_args() { tokenize(this->args); }

void AtomDBBrokerProxy::tokenize(vector<string>& output) {
    output.insert(output.begin(), this->tokens.begin(), this->tokens.end());
    output.insert(output.begin(), std::to_string(this->tokens.size()));
    BaseProxy::tokenize(output);
}

bool AtomDBBrokerProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in " << this->my_id());
    if (BaseQueryProxy::from_remote_peer(command, args)) {
        return true;
    } else if (command == AtomDBBrokerProxy::ADD_ATOMS) {
        add_atoms();
        return true;
    } else {
        Utils::error("Invalid AtomDBBrokerProxy command: <" + command + ">");
        return false;
    }
}

void AtomDBBrokerProxy::add_atoms(vector<Atom>) {}

vector<Atom> AtomDBBrokerProxy::build_atoms_from_args(vector<string> args) {
    return vector<Atom>();
}

