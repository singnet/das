#include "AtomDBUtils.h"

#include "AtomDBSingleton.h"
#include "Hasher.h"
#include "Logger.h"
#include "MettaMapping.h"

using namespace atomdb;
using namespace commons;

AtomDBUtils::AtomDBUtils() {}

AtomDBUtils::~AtomDBUtils() {}

// -------------------------------------------------------------------------------------------------
// Public methods

void AtomDBUtils::reachable_terminal_set(set<string>& output,
                                         const string& handle,
                                         bool metta_mapping,
                                         bool name_flag) {
    STACK_TRACE();
    if (Hasher::is_node_handle(handle)) {
        auto atom = AtomDBSingleton::get_instance()->get_atom(handle);
        if (atom == nullptr) {
            return;
        }
        if (name_flag) {
            auto node = dynamic_pointer_cast<Node>(atom);
            if (node != nullptr) {
                output.insert(node->name);
            }
        } else {
            output.insert(handle);
        }
        return;
    }
    if (Hasher::is_link_handle(handle)) {
        auto atom = AtomDBSingleton::get_instance()->get_atom(handle);
        auto link = dynamic_pointer_cast<Link>(atom);
        if (link != nullptr) {
            AtomDBUtils::reachable_terminal_set_recursive(output, link, metta_mapping, name_flag);
        }
    }
}

void AtomDBUtils::count_reachable_atoms(const string& handle,
                                        size_t& node_count,
                                        size_t& link_count,
                                        size_t& atom_count,
                                        shared_ptr<Keychain> keychain,
                                        set<string>& visited) {
    if (!visited.insert(handle).second) {
        return;
    }
    auto atomdb = AtomDBSingleton::get_instance();
    auto key_sensitive_atomdb = dynamic_pointer_cast<KeySensitiveAtomDB>(atomdb);
    auto atom = key_sensitive_atomdb ? key_sensitive_atomdb->get_atom(handle, keychain)
                                     : atomdb->get_atom(handle);
    if (atom != nullptr) {
        if (Hasher::is_link_handle(handle)) {
            auto link = dynamic_pointer_cast<Link>(atom);
            if (link != nullptr) {
                AtomDBUtils::count_reachable_atoms_recursive(
                    link, node_count, link_count, atom_count, keychain, visited);
                link_count++;
            }
        } else {
            node_count++;
        }
    }
    atom_count++;
}

string AtomDBUtils::handle_to_metta(const string& handle, shared_ptr<Keychain> keychain) {
    STACK_TRACE();
    map<string, string> not_used;
    shared_ptr<AtomDB> atomdb = AtomDBSingleton::get_instance();
    shared_ptr<KeySensitiveAtomDB> key_sensitive_atomdb =
        dynamic_pointer_cast<KeySensitiveAtomDB>(atomdb);
    return handle_to_metta_recursive(handle, not_used, false, keychain, atomdb, key_sensitive_atomdb);
}

string AtomDBUtils::handle_to_metta(const string& handle,
                                    map<string, string>& mapping,
                                    shared_ptr<Keychain> keychain) {
    STACK_TRACE();
    shared_ptr<AtomDB> atomdb = AtomDBSingleton::get_instance();
    shared_ptr<KeySensitiveAtomDB> key_sensitive_atomdb =
        dynamic_pointer_cast<KeySensitiveAtomDB>(atomdb);
    return handle_to_metta_recursive(handle, mapping, true, keychain, atomdb, key_sensitive_atomdb);
}

shared_ptr<Atom> AtomDBUtils::get_atom(const string& handle, shared_ptr<Keychain> keychain) {
    STACK_TRACE();
    shared_ptr<AtomDB> atomdb = AtomDBSingleton::get_instance();
    shared_ptr<KeySensitiveAtomDB> key_sensitive_atomdb =
        dynamic_pointer_cast<KeySensitiveAtomDB>(atomdb);
    auto atom = (key_sensitive_atomdb ? key_sensitive_atomdb->get_atom(handle, keychain)
                                      : atomdb->get_atom(handle));
    return atom;
}

shared_ptr<Node> AtomDBUtils::get_node(const string& handle, shared_ptr<Keychain> keychain) {
    STACK_TRACE();
    if (!Hasher::is_node_handle(handle)) return nullptr;
    return dynamic_pointer_cast<Node>(AtomDBUtils::get_atom(handle, keychain));
}

string AtomDBUtils::get_node_name(const string& handle, shared_ptr<Keychain> keychain) {
    STACK_TRACE();
    auto node = AtomDBUtils::get_node(handle, keychain);
    if (node == nullptr) {
        return "";
    } else {
        return node->name;
    }
}

double AtomDBUtils::get_strength(const string& handle,
                                 shared_ptr<Keychain> keychain,
                                 const string& strength_tag) {
    STACK_TRACE();
    double answer = 1.0;
    auto atom = AtomDBUtils::get_atom(handle, keychain);
    if (atom == nullptr) {
        RAISE_ERROR("Atom does not exist: " + handle);
    } else {
        answer = atom->custom_attributes.get_or<double>(strength_tag, 1.0);
    }
    return answer;
}

// -------------------------------------------------------------------------------------------------
// Private methods

void AtomDBUtils::reachable_terminal_set_recursive(set<string>& output,
                                                   shared_ptr<Link> link,
                                                   bool metta_mapping,
                                                   bool name_flag) {
    bool first_target = true;
    for (string& target_handle : link->targets) {
        if (Hasher::is_node_handle(target_handle)) {
            if (!(metta_mapping && first_target)) {
                auto atom = AtomDBSingleton::get_instance()->get_atom(target_handle);
                if (atom != nullptr) {
                    if (name_flag) {
                        auto node = dynamic_pointer_cast<Node>(atom);
                        if (node != nullptr) {
                            output.insert(node->name);
                        }
                    } else {
                        output.insert(target_handle);
                    }
                }
            }
        } else if (Hasher::is_link_handle(target_handle)) {
            auto atom = AtomDBSingleton::get_instance()->get_atom(target_handle);
            auto nested_link = dynamic_pointer_cast<Link>(atom);
            if (nested_link != nullptr) {
                AtomDBUtils::reachable_terminal_set_recursive(
                    output, nested_link, metta_mapping, name_flag);
            }
        }
        first_target = false;
    }
}

void AtomDBUtils::count_reachable_atoms_recursive(shared_ptr<Link> link,
                                                  size_t& node_count,
                                                  size_t& link_count,
                                                  size_t& atom_count,
                                                  shared_ptr<Keychain> keychain,
                                                  set<string>& visited) {
    auto atomdb = AtomDBSingleton::get_instance();
    auto key_sensitive_atomdb = dynamic_pointer_cast<KeySensitiveAtomDB>(atomdb);

    for (string& target_handle : link->targets) {
        if (!visited.insert(target_handle).second) {
            continue;
        }
        auto atom = key_sensitive_atomdb ? key_sensitive_atomdb->get_atom(target_handle, keychain)
                                         : atomdb->get_atom(target_handle);
        if (atom != nullptr) {
            if (Hasher::is_link_handle(target_handle)) {
                auto nested_link = dynamic_pointer_cast<Link>(atom);
                if (nested_link != nullptr) {
                    AtomDBUtils::count_reachable_atoms_recursive(
                        nested_link, node_count, link_count, atom_count, keychain, visited);
                    link_count++;
                }
            } else {
                node_count++;
            }
        }
        atom_count++;
    }
}

string AtomDBUtils::handle_to_metta_recursive(const string& handle,
                                              map<string, string>& mapping,
                                              bool populate_map,
                                              shared_ptr<Keychain> keychain,
                                              shared_ptr<AtomDB> atomdb,
                                              shared_ptr<KeySensitiveAtomDB> key_sensitive_atomdb) {
    string answer = "";
    auto iterator = mapping.find(handle);
    if (iterator != mapping.end()) {
        answer = iterator->second;
    } else if (!Hasher::is_atom_handle(handle)) {
        RAISE_ERROR("Unknown handle in handle_to_metta(): " + handle);
    } else {
        shared_ptr<Atom> atom = nullptr;

        if (key_sensitive_atomdb != nullptr &&
            atomdb->get_protection_mode() != atomdb_api_types::ProtectionMode::UNPROTECTED) {
            // AtomDB is protected. Keychain must be forwarded.
            if (keychain != nullptr) {
                atom = key_sensitive_atomdb->get_atom(handle, keychain);
            } else {
                RAISE_ERROR("AtomDB is protected and requires a keychain");
            }
        } else {
            // AtomDB is not protected. Disregard keychain.
            atom = atomdb->get_atom(handle);
        }

        if (atom != nullptr) {
            string expression_tag = atom->type;
            if (Hasher::is_node_handle(handle)) {
                if (expression_tag == MettaMapping::SYMBOL_NODE_TYPE) {
                    answer = dynamic_pointer_cast<Node>(atom)->name;
                } else {
                    RAISE_ERROR("Node type \"" + expression_tag + "\" can't be mapped to MeTTa");
                }
            } else {
                if (expression_tag == MettaMapping::EXPRESSION_LINK_TYPE) {
                    vector<string> targets;
                    for (string& _handle : dynamic_pointer_cast<Link>(atom)->targets) {
                        targets.push_back(handle_to_metta_recursive(
                            _handle, mapping, populate_map, keychain, atomdb, key_sensitive_atomdb));
                    }
                    answer = MettaMapping::metta_expr(targets);
                } else {
                    RAISE_ERROR("Link type \"" + expression_tag + "\" can't be mapped to MeTTa");
                }
            }
        } else {
            RAISE_ERROR("Unknown handle in handle_to_metta(): " + handle);
        }
        if (populate_map) {
            mapping[handle] = answer;
        }
    }
    return answer;
}
