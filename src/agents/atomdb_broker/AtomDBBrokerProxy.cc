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
// Constructors, destructors and initialization

AtomDBBrokerProxy::AtomDBBrokerProxy() : BaseProxy() {
    // Typically used in processor
    LOG_INFO("[[ AtomDBBrokerProxy Server ON ]]");
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::ATOMDB;
    this->atomdb = AtomDBSingleton::get_instance();
    this->keep_alive = true;
}

AtomDBBrokerProxy::AtomDBBrokerProxy(string name) : BaseProxy() {
    LOG_INFO("[[ AtomDBBrokerProxy Client ON ]]");
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::ATOMDB;
    this->keep_alive = true;
}

AtomDBBrokerProxy::~AtomDBBrokerProxy() {}

// -------------------------------------------------------------------------------------------------
// Client-side API
void AtomDBBrokerProxy::shutdown() { to_remote_peer(SHUTDOWN, {}); }

bool AtomDBBrokerProxy::running() { return this->keep_alive; }

void AtomDBBrokerProxy::pack_command_line_args() { tokenize(this->args); }

void AtomDBBrokerProxy::tokenize(vector<string>& output) {
    // output.insert(output.begin(), this->tokens.begin(), this->tokens.end());
    // output.insert(output.begin(), std::to_string(this->tokens.size()));
    BaseProxy::tokenize(output);
}

vector<string> AtomDBBrokerProxy::add_atoms(const vector<Atom*>& atoms) {
    vector<string> args;
    string atom_type;
    LOG_INFO("[[ add_atoms 1 ]]");
    for (Atom* atom : atoms) {
        atom->tokenize(args);
        
        if (dynamic_cast<Node*>(atom)) {
            atom_type = "NODE";
        } else if (dynamic_cast<Link*>(atom)) {
            atom_type = "LINK";
        } else {
            Utils::error("Invalid type");
        }

        args.insert(args.begin(), atom_type);
    }
    LOG_INFO("[[ add_atoms 2 ]]");
    for (auto a : args) {
        cout << "--> " << a << endl;
    }
    to_remote_peer(AtomDBBrokerProxy::ADD_ATOMS, args);
    LOG_INFO("[[ add_atoms 3 ]]");
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void AtomDBBrokerProxy::untokenize(vector<string>& tokens) {
    BaseProxy::untokenize(tokens);
    // unsigned int num_tokens = std::stoi(tokens[0]);
    // this->tokens.insert(this->tokens.begin(), tokens.begin() + 1, tokens.begin() + 1 + num_tokens);
    // tokens.erase(tokens.begin(), tokens.begin() + 1 + num_tokens);
}

bool AtomDBBrokerProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in " << this->my_id());
    if (BaseProxy::from_remote_peer(command, args)) {
        return true;
    } else if (command == AtomDBBrokerProxy::ADD_ATOMS) {
        piggyback_add_atoms(args);
        return true;
    } else if (command == AtomDBBrokerProxy::SHUTDOWN) {
        this->keep_alive = false;
        return true;
    } else {
        Utils::error("Invalid AtomDBBrokerProxy command: <" + command + ">");
        return false;
    }
}

// Temporary Name
vector<string> AtomDBBrokerProxy::piggyback_add_atoms(vector<string> tokens) {
    try {
        vector<Atom*> atoms = build_atoms_from_tokens(tokens);
        auto ret = this->atomdb->add_atoms(atoms);
        to_remote_peer(BaseProxy::FINISHED, {});
    } catch (const std::runtime_error& exception) {
        raise_error_on_peer(exception.what());
    } catch (const std::exception& exception) {
        raise_error_on_peer(exception.what());
    }
}

vector<Atom*> AtomDBBrokerProxy::build_atoms_from_tokens(vector<string> tokens) {
    // Token node: Symbol false 3 is_literal bool true human 
    // Token link: Expression false 6 confidence double 0.230000 strengh double 0.950000 3 target1 target2 target3 

    vector<Atom*> atoms;
    string current;
    vector<string> buffer;

    auto flush = [&]() {
        if (current.empty()) return;
        if (current == "NODE") {
            auto n = new Node(buffer);
            atoms.push_back(std::move(n));
        } else { // LINK
            auto l = new Link(buffer);
            atoms.push_back(std::move(l));
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

// vector<Atom*> AtomDBBrokerProxy::build_atoms_from_tokens(vector<string> tokens) {

//     vector<Atom*> atoms;
//     string current;
//     vector<string> temp_tokens; 

//     for (const auto& a : tokens) {
//         if (a == "NODE" || a == "LINK") {
//             if (current.empty()) {
//                 current = a;
//             } else {
//                 if (current == "NODE") {
//                     string type = temp_tokens[0];
//                     string name = temp_tokens[1];
//                     if (temp_tokens.size() > 2) {
//                         Properties attrs;
//                         auto num_attrs = std::stoi(temp_tokens[2]);
//                         for (int i = 2; i < num_attrs + 1;  i += 2) {
//                             attrs[temp_tokens[i + 1]] = temp_tokens[i + 2];
//                         }
//                         atoms.push_back(new Node(type, name, attrs));
//                     } else {
//                         atoms.push_back(new Node(type, name));
//                     }
//                 } else {
//                     string type = temp_tokens[0];
//                     auto targets_size = std::stoi(temp_tokens[1]);
//                     vector<string> targets;
//                     for (int i = 2; i < targets_size + 2; i++) {
//                         targets.push_back(temp_tokens[i]);
//                     }
//                     bool is_toplevel = (temp_tokens[2 + targets_size] == "true");
                    
//                     // What should be the link size without attributes
//                     int link_size_without_attrs = 3 + targets_size;

//                     if (temp_tokens.size() > link_size_without_attrs) {
//                         Properties attrs;
//                         int start_idx = link_size_without_attrs;
//                         // auto num_attrs = std::stoi(temp_tokens[start_idx]);
//                         for (int i = start_idx; i < temp_tokens.size() - 1;  i += 2) {
//                             attrs[temp_tokens[i + 1]] = temp_tokens[i + 2];
//                         }
//                         atoms.push_back(new Link(type, targets, is_toplevel, attrs));
//                     } else {
//                         atoms.push_back(new Link(type, targets, is_toplevel));
//                     }
//                 }
//                 temp_tokens.clear();
//                 current = a;
//             } 
//         } else {
//             temp_tokens.push_back(a);
//         }
//     }

//     if (!temp_tokens.empty() && !current.empty()) {
//         if (current == "NODE") {
//             string type = temp_tokens[0];
//             string name = temp_tokens[1];
//             if (temp_tokens.size() > 2) {
//                 Properties attrs;
//                 auto num_attrs = std::stoi(temp_tokens[2]);
//                 for (int i = 2; i < num_attrs + 1;  i += 2) {
//                     attrs[temp_tokens[i + 1]] = temp_tokens[i + 2];
//                 }
//                 atoms.push_back(new Node(type, name, attrs));
//             } else {
//                 atoms.push_back(new Node(type, name));
//             }
//         } else {
//             string type = temp_tokens[0];
//             auto targets_size = std::stoi(temp_tokens[1]);
//             vector<string> targets;
//             for (int i = 2; i < targets_size + 2; i++) {
//                 targets.push_back(temp_tokens[i]);
//             }
//             bool is_toplevel = (temp_tokens[2 + targets_size] == "true");
            
//             // What should be the link size without attributes
//             int link_size_without_attrs = 3 + targets_size;

//             if (temp_tokens.size() > link_size_without_attrs) {
//                 Properties attrs;
//                 int start_idx = link_size_without_attrs;
//                 // auto num_attrs = std::stoi(temp_tokens[start_idx]);
//                 for (int i = start_idx; i < temp_tokens.size() - 1;  i += 2) {
//                     attrs[temp_tokens[i + 1]] = temp_tokens[i + 2];
//                 }
//                 atoms.push_back(new Link(type, targets, is_toplevel, attrs));
//             } else {
//                 atoms.push_back(new Link(type, targets, is_toplevel));
//             }
//         }
//     }

//     return atoms;
// }