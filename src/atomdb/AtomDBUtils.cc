#include "AtomDBUtils.h"

#include "AtomDBSingleton.h"
#include "Hasher.h"
#include "Logger.h"
#include "MettaMapping.h"

using namespace atomdb;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Public methods

void AtomDBUtils::reachable_terminal_set(set<string>& output,
                                         const string& handle,
                                         bool name_flag,
                                         shared_ptr<Keychain> keychain) {
    STACK_TRACE();
    if (Hasher::is_node_handle(handle)) {
        string name = get_node_name(handle, keychain);
        if (name != "") {
            if (name_flag) {
                output.insert(name);
            } else {
                output.insert(handle);
            }
        }
    } else if (Hasher::is_link_handle(handle)) {
        for (string& target : get_link_targets(handle, keychain)) {
            reachable_terminal_set(output, target, name_flag, keychain);
        }
    } else {
        RAISE_ERROR("Invalid handle: " + handle);
    }
}

void AtomDBUtils::count_reachable_atoms(const string& handle,
                                        size_t& node_count,
                                        size_t& link_count,
                                        size_t& atom_count,
                                        shared_ptr<Keychain> keychain,
                                        set<string>& visited) {
    STACK_TRACE();
    if (visited.insert(handle).second) {
        if (Hasher::is_link_handle(handle)) {
            for (string& target : get_link_targets(handle, keychain)) {
                count_reachable_atoms(target, node_count, link_count, atom_count, keychain, visited);
            }
            link_count++;
        } else {
            node_count++;
        }
        atom_count++;
    }
}

string AtomDBUtils::handle_to_metta(const string& handle, shared_ptr<Keychain> keychain) {
    STACK_TRACE();
    map<string, string> not_used;
    return handle_to_metta_recursive(handle, not_used, false, keychain);
}

string AtomDBUtils::handle_to_metta(const string& handle,
                                    map<string, string>& mapping,
                                    shared_ptr<Keychain> keychain) {
    STACK_TRACE();
    return handle_to_metta_recursive(handle, mapping, true, keychain);
}

shared_ptr<Atom> AtomDBUtils::get_atom(const string& handle, shared_ptr<Keychain> keychain) {
    STACK_TRACE();
    shared_ptr<Atom> atom = nullptr;
    shared_ptr<AtomDB> atomdb = AtomDBSingleton::get_instance();
    if (atomdb != nullptr) {
        shared_ptr<KeySensitiveAtomDB> key_sensitive_atomdb =
            dynamic_pointer_cast<KeySensitiveAtomDB>(atomdb);
        atom = (key_sensitive_atomdb != nullptr ? key_sensitive_atomdb->get_atom(handle, keychain)
                                                : atomdb->get_atom(handle));
    } else {
        RAISE_ERROR("AtomDB is not available");
    }
    return atom;
}

shared_ptr<Node> AtomDBUtils::get_node(const string& handle, shared_ptr<Keychain> keychain) {
    STACK_TRACE();
    if (!Hasher::is_node_handle(handle)) return nullptr;
    auto atom = AtomDBUtils::get_atom(handle, keychain);
    if (atom == nullptr) return nullptr;
    return dynamic_pointer_cast<Node>(atom);
}

shared_ptr<Link> AtomDBUtils::get_link(const string& handle, shared_ptr<Keychain> keychain) {
    STACK_TRACE();
    if (!Hasher::is_link_handle(handle)) return nullptr;
    return dynamic_pointer_cast<Link>(AtomDBUtils::get_atom(handle, keychain));
}

string AtomDBUtils::get_node_name(const string& handle, shared_ptr<Keychain> keychain) {
    STACK_TRACE();
    string answer = "";
    if (Hasher::is_node_handle(handle)) {
        auto atom = AtomDBUtils::get_atom(handle, keychain);
        if (atom != nullptr) {
            auto node = dynamic_pointer_cast<Node>(atom);
            if (node != nullptr) {
                answer = node->name;
            }
        }
    }
    return answer;
}

vector<string> AtomDBUtils::get_link_targets(const string& handle, shared_ptr<Keychain> keychain) {
    STACK_TRACE();
    vector<string> answer;
    if (Hasher::is_link_handle(handle)) {
        shared_ptr<AtomDB> atomdb = AtomDBSingleton::get_instance();
        shared_ptr<KeySensitiveAtomDB> key_sensitive_atomdb =
            dynamic_pointer_cast<KeySensitiveAtomDB>(atomdb);
        shared_ptr<atomdb_api_types::HandleList> targets;
        if (key_sensitive_atomdb == nullptr) {
            targets = atomdb->query_for_targets(handle);
        } else {
            targets = key_sensitive_atomdb->query_for_targets(handle, keychain);
        }
        if (targets != nullptr) {
            unsigned int arity = targets->size();
            for (unsigned int i = 0; i < arity; i++) {
                answer.push_back(targets->get_handle(i));
            }
        }
    }
    return answer;
}

double AtomDBUtils::get_strength(const string& handle,
                                 shared_ptr<Keychain> keychain,
                                 const string& strength_tag) {
    STACK_TRACE();
    double answer = 0.0;
    auto atom = AtomDBUtils::get_atom(handle, keychain);
    if (atom != nullptr) {
        answer = atom->custom_attributes.get_or<double>(strength_tag, 1.0);
    }
    return answer;
}

// -------------------------------------------------------------------------------------------------
// HandleDecoder API

shared_ptr<Atom> AtomDBUtils::decode_atom(const string& handle, shared_ptr<Keychain> keychain) {
    return get_atom(handle, keychain);
}

string AtomDBUtils::decode_node_name(const string& handle, shared_ptr<Keychain> keychain) {
    return get_node_name(handle, keychain);
}

vector<string> AtomDBUtils::decode_link_targets(const string& handle, shared_ptr<Keychain> keychain) {
    return get_link_targets(handle, keychain);
}

// -------------------------------------------------------------------------------------------------
// Private methods

string AtomDBUtils::handle_to_metta_recursive(const string& handle,
                                              map<string, string>& mapping,
                                              bool populate_map,
                                              shared_ptr<Keychain> keychain) {
    string answer = "";
    auto iterator = mapping.find(handle);
    if (iterator != mapping.end()) {
        answer = iterator->second;
    } else if (!Hasher::is_atom_handle(handle)) {
        RAISE_ERROR("Unknown handle in handle_to_metta(): " + handle);
    } else {
        if (Hasher::is_node_handle(handle)) {
            answer = get_node_name(handle, keychain);
            if (answer == "") {
                RAISE_ERROR("Invalid node handle: " + handle);
            }
        } else {
            vector<string> targets;
            for (string& _handle : get_link_targets(handle, keychain)) {
                targets.push_back(handle_to_metta_recursive(_handle, mapping, populate_map, keychain));
            }
            if (targets.size() == 0) {
                RAISE_ERROR("Invalid link handle: " + handle);
            }
            answer = MettaMapping::metta_expr(targets);
        }
        if (populate_map) {
            mapping[handle] = answer;
        }
    }
    return answer;
}
