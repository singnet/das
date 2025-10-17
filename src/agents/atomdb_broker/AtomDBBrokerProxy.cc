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
string AtomDBBrokerProxy::ADD_ATOMS_RESPONSE = "add_atoms_response";
string AtomDBBrokerProxy::ADD_ATOMS_ERROR = "add_atoms_error";
string AtomDBBrokerProxy::SHUTDOWN = "shutdown";

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

AtomDBBrokerProxy::AtomDBBrokerProxy() : BaseProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::ATOMDB;
    this->atomdb = AtomDBSingleton::get_instance();
    this->keep_alive_flag = true;
    this->add_atoms_response_flag = false;
    this->add_atoms_error_flag = false;
}

AtomDBBrokerProxy::~AtomDBBrokerProxy() {}

// -------------------------------------------------------------------------------------------------
// Client-side API
void AtomDBBrokerProxy::shutdown() { to_remote_peer(SHUTDOWN, {}); }

bool AtomDBBrokerProxy::running() { return this->keep_alive_flag; }

void AtomDBBrokerProxy::pack_command_line_args() { tokenize(this->args); }

void AtomDBBrokerProxy::tokenize(vector<string>& output) {
    BaseProxy::tokenize(output);
}

vector<string> AtomDBBrokerProxy::add_atoms(const vector<Atom*>& atoms) {
    vector<string> args;
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
    }
    
    // {
    //     lock_guard<mutex> semaphore(this->api_mutex);
    //     this->args = args;
    // }


    to_remote_peer(AtomDBBrokerProxy::ADD_ATOMS, args);


    while (true) {
        // {
        //     lock_guard<mutex> semaphore(this->api_mutex);
        if (this->add_atoms_error_flag) {
            return {};
        }

        if (this->add_atoms_response_flag && !this->add_atoms_response.empty()) {
            auto resp = this->add_atoms_response;

            // clean
            this->add_atoms_response.clear();
            this->add_atoms_response_flag = false;
            this->add_atoms_error_flag = false;
            // this->args.clear();

            return resp;
        }
        // }
        Utils::sleep();
    }
    
    // // wait for response (early-return on processor error)
    // while (!this->add_atoms_response_flag && this->add_atoms_response.empty()) {
    //     if (this->add_atoms_error_flag) return {};
    //     Utils::sleep();
    // }
    // return this->add_atoms_response;
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void AtomDBBrokerProxy::untokenize(vector<string>& tokens) {
    BaseProxy::untokenize(tokens);
}

bool AtomDBBrokerProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in " << this->my_id());
   
    if (BaseProxy::from_remote_peer(command, args)) {
        return true;
    } else if (command == AtomDBBrokerProxy::ADD_ATOMS) {
        piggyback_add_atoms(args);
        return true;
    } else if (command == AtomDBBrokerProxy::ADD_ATOMS_RESPONSE) {
        this->add_atoms_response_flag = true;
        this->add_atoms_response.insert(this->add_atoms_response.begin(), args.begin(), args.end());
        return true;
    } else if (command == AtomDBBrokerProxy::ADD_ATOMS_ERROR) {
        cout << "---> Chegeui aqio" << endl;
        for (auto a : args) {
            cout << "++++>>> " << a << endl;
        }
        this->add_atoms_error_flag = true;
        return true;
    } else if (command == AtomDBBrokerProxy::SHUTDOWN) {
        this->keep_alive_flag = false;
        return true;
    } else {
        Utils::error("Invalid AtomDBBrokerProxy command: <" + command + ">");
        return false;
    }
}

void AtomDBBrokerProxy::piggyback_add_atoms(const vector<string>& tokens) {
    try {
        vector<Atom*> atoms = build_atoms_from_tokens(tokens);
        auto ret = this->atomdb->add_atoms(atoms);
        to_remote_peer(AtomDBBrokerProxy::ADD_ATOMS_RESPONSE, ret);
    } catch (const std::runtime_error& exception) {
        to_remote_peer(AtomDBBrokerProxy::ADD_ATOMS_ERROR, {exception.what()});
    } catch (const std::exception& exception) {
        to_remote_peer(AtomDBBrokerProxy::ADD_ATOMS_ERROR, {exception.what()});
    }
}

vector<Atom*> AtomDBBrokerProxy::build_atoms_from_tokens(const vector<string>& tokens) {
    // Token node: Symbol false 3 is_literal bool true human 
    // Token link: Expression false 6 confidence double 0.230000 strengh double 0.950000 3 target1 target2 target3 

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
