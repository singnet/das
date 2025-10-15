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

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

AtomDBBrokerProxy::AtomDBBrokerProxy() : BaseProxy() {
    // Typically used in processor
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::ATOMDB;
    this->atomdb = AtomDBSingleton::get_instance();
}

AtomDBBrokerProxy::AtomDBBrokerProxy(const string& action, const vector<string>& tokens) : BaseProxy() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->command = ServiceBus::ATOMDB;
    this->atomdb = AtomDBSingleton::get_instance();
    this->action = action;
    this->tokens = tokens;
}

AtomDBBrokerProxy::~AtomDBBrokerProxy() {}

// -------------------------------------------------------------------------------------------------
// Client-side API

void AtomDBBrokerProxy::pack_command_line_args() { tokenize(this->args); }

const string AtomDBBrokerProxy::get_action() { return this->action; }

void AtomDBBrokerProxy::tokenize(vector<string>& output) {
    output.insert(output.begin(), this->tokens.begin(), this->tokens.end());
    output.insert(output.begin(), std::to_string(this->tokens.size()));
    output.insert(output.begin(), this->action);
    BaseProxy::tokenize(output);
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void AtomDBBrokerProxy::untokenize(vector<string>& tokens) {
    BaseProxy::untokenize(tokens);
    this->action = tokens[0];
    unsigned int num_tokens = std::stoi(tokens[1]);
    this->tokens.insert(this->tokens.begin(), tokens.begin() + 2, tokens.begin() + 2 + num_tokens);
    tokens.erase(tokens.begin(), tokens.begin() + 2 + num_tokens);
}

bool AtomDBBrokerProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in " << this->my_id());
    if (BaseProxy::from_remote_peer(command, args)) {
        return true;
    } else if (command == AtomDBBrokerProxy::ADD_ATOMS) {
        add_atoms();
        return true;
    } else {
        Utils::error("Invalid AtomDBBrokerProxy command: <" + command + ">");
        return false;
    }
}

void AtomDBBrokerProxy::add_atoms() {
    try {
        vector<Atom*> atoms = build_atoms_from_tokens();
        this->atomdb->add_atoms(atoms);
    } catch (const std::runtime_error& exception) {
        raise_error_on_peer(exception.what());
    } catch (const std::exception& exception) {
        raise_error_on_peer(exception.what());
    }
    to_remote_peer(BaseProxy::FINISHED, {});
}

vector<Atom*> AtomDBBrokerProxy::build_atoms_from_tokens() {
    vector<Atom*> atoms;
    string current;
    vector<string> tokens; 

    for (const auto& a : this->tokens) {
        if (a == "NODE" || a == "LINK") {
            if (current.empty()) {
                current = a;
            } else {
                if (current == "NODE") {
                    string type = tokens[0];
                    string name = tokens[1];
                    if (tokens.size() > 2) {
                        Properties attrs;
                        auto num_attrs = std::stoi(tokens[2]);
                        for (int i = 2; i < num_attrs + 1;  i += 2) {
                            attrs[tokens[i + 1]] = tokens[i + 2];
                        }
                        atoms.push_back(new Node(type, name, attrs));
                    } else {
                        atoms.push_back(new Node(type, name));
                    }
                } else {
                    string type = tokens[0];
                    auto targets_size = std::stoi(tokens[1]);
                    vector<string> targets;
                    for (int i = 2; i < targets_size + 2; i++) {
                        targets.push_back(tokens[i]);
                    }
                    bool is_toplevel = (tokens[2 + targets_size] == "true");
                    
                    // What should be the link size without attributes
                    int link_size_without_attrs = 3 + targets_size;

                    if (tokens.size() > link_size_without_attrs) {
                        Properties attrs;
                        int start_idx = link_size_without_attrs;
                        // auto num_attrs = std::stoi(tokens[start_idx]);
                        for (int i = start_idx; i < tokens.size() - 1;  i += 2) {
                            attrs[tokens[i + 1]] = tokens[i + 2];
                        }
                        atoms.push_back(new Link(type, targets, is_toplevel, attrs));
                    } else {
                        atoms.push_back(new Link(type, targets, is_toplevel));
                    }
                }
                tokens.clear();
                current = a;
            } 
        } else {
            tokens.push_back(a);
        }
    }

    if (!tokens.empty() && !current.empty()) {
        if (current == "NODE") {
            string type = tokens[0];
            string name = tokens[1];
            if (tokens.size() > 2) {
                Properties attrs;
                auto num_attrs = std::stoi(tokens[2]);
                for (int i = 2; i < num_attrs + 1;  i += 2) {
                    attrs[tokens[i + 1]] = tokens[i + 2];
                }
                atoms.push_back(new Node(type, name, attrs));
            } else {
                atoms.push_back(new Node(type, name));
            }
        } else {
            string type = tokens[0];
            auto targets_size = std::stoi(tokens[1]);
            vector<string> targets;
            for (int i = 2; i < targets_size + 2; i++) {
                targets.push_back(tokens[i]);
            }
            bool is_toplevel = (tokens[2 + targets_size] == "true");
            
            // What should be the link size without attributes
            int link_size_without_attrs = 3 + targets_size;

            if (tokens.size() > link_size_without_attrs) {
                Properties attrs;
                int start_idx = link_size_without_attrs;
                // auto num_attrs = std::stoi(tokens[start_idx]);
                for (int i = start_idx; i < tokens.size() - 1;  i += 2) {
                    attrs[tokens[i + 1]] = tokens[i + 2];
                }
                atoms.push_back(new Link(type, targets, is_toplevel, attrs));
            } else {
                atoms.push_back(new Link(type, targets, is_toplevel));
            }
        }
    }

    return atoms;
}

