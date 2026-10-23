#include "AtomDBUtils.h"

#include <iostream>

#include "AtomDBSingleton.h"
#include "Logger.h"
#include "MettaMapping.h"

using namespace atomdb;

AtomDBUtils::AtomDBUtils() {}

AtomDBUtils::~AtomDBUtils() {}

// -------------------------------------------------------------------------------------------------
// Public methods

void AtomDBUtils::reachable_terminal_set(set<string>& output, const string& handle, bool metta_mapping) {
    auto atom = AtomDBSingleton::get_instance()->get_atom(handle);
    if (atom != nullptr) {
        if (Atom::is_node(atom)) {
            output.insert(handle);
        } else {
            AtomDBUtils::reachable_terminal_set_recursive(
                output, dynamic_pointer_cast<Link>(atom), metta_mapping);
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
        auto link = dynamic_pointer_cast<Link>(atom);
        if (link == nullptr) {
            node_count++;
        } else {
            AtomDBUtils::count_reachable_atoms_recursive(
                link, node_count, link_count, atom_count, keychain, visited);
            link_count++;
        }
    }
    atom_count++;
}

string AtomDBUtils::handle_to_metta(const string& handle, shared_ptr<Keychain> keychain) {
    map<string, string> not_used;
    shared_ptr<AtomDB> atomdb = AtomDBSingleton::get_instance();
    shared_ptr<KeySensitiveAtomDB> key_sensitive_atomdb =
        dynamic_pointer_cast<KeySensitiveAtomDB>(atomdb);
    return handle_to_metta_recursion(handle, not_used, false, keychain, atomdb, key_sensitive_atomdb);
}

string AtomDBUtils::handle_to_metta(const string& handle,
                                    map<string, string>& mapping,
                                    shared_ptr<Keychain> keychain) {
    shared_ptr<AtomDB> atomdb = AtomDBSingleton::get_instance();
    shared_ptr<KeySensitiveAtomDB> key_sensitive_atomdb =
        dynamic_pointer_cast<KeySensitiveAtomDB>(atomdb);
    return handle_to_metta_recursion(handle, mapping, true, keychain, atomdb, key_sensitive_atomdb);
}

// -------------------------------------------------------------------------------------------------
// Private methods

void AtomDBUtils::reachable_terminal_set_recursive(set<string>& output,
                                                   shared_ptr<Link> link,
                                                   bool metta_mapping) {
    bool first_target = true;
    for (string& target_handle : link->targets) {
        auto atom = AtomDBSingleton::get_instance()->get_atom(target_handle);
        if (Atom::is_node(atom)) {
            if (!(metta_mapping && first_target)) {
                output.insert(atom->handle());
            }
        } else {
            AtomDBUtils::reachable_terminal_set_recursive(
                output, dynamic_pointer_cast<Link>(atom), metta_mapping);
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
            auto link = dynamic_pointer_cast<Link>(atom);
            if (link == nullptr) {
                node_count++;
            } else {
                AtomDBUtils::count_reachable_atoms_recursive(
                    link, node_count, link_count, atom_count, keychain, visited);
                link_count++;
            }
        }
        atom_count++;
    }
}

string AtomDBUtils::handle_to_metta_recursion(const string& handle,
                                              map<string, string>& mapping,
                                              bool populate_map,
                                              shared_ptr<Keychain> keychain,
                                              shared_ptr<AtomDB> atomdb,
                                              shared_ptr<KeySensitiveAtomDB> key_sensitive_atomdb) {
    cout << "[handle_to_metta] handle=" << handle
         << " uid=" << (atomdb ? atomdb->get_uid() : "<null-atomdb>")
         << " populate_map=" << populate_map
         << " key_sensitive=" << (key_sensitive_atomdb != nullptr)
         << " keychain=" << (keychain != nullptr)
         << " protection_mode="
         << (atomdb ? static_cast<int>(atomdb->get_protection_mode()) : -1) << endl;
    if (keychain != nullptr && atomdb != nullptr) {
        cout << "[handle_to_metta] public_key='" << keychain->get_public_key(atomdb->get_uid()) << "'"
             << endl;
    }

    string answer = "";
    auto iterator = mapping.find(handle);
    if (iterator != mapping.end()) {
        cout << "[handle_to_metta] mapping HIT handle=" << handle << " value=" << iterator->second
             << endl;
        answer = iterator->second;
    } else {
        cout << "[handle_to_metta] mapping MISS handle=" << handle << endl;
        shared_ptr<Atom> atom = nullptr;

        if (key_sensitive_atomdb != nullptr && atomdb->get_protection_mode() != atomdb_api_types::ProtectionMode::UNPROTECTED) {
            // AtomDB is protected. Keychain must be forwarded.
            if (keychain != nullptr) {
                cout << "[handle_to_metta] path=PROTECTED get_atom(handle, keychain)" << endl;
                atom = key_sensitive_atomdb->get_atom(handle, keychain);
            } else {
                cout << "[handle_to_metta] path=PROTECTED but keychain is null" << endl;
                RAISE_ERROR("AtomDB is protected and requires a keychain");
            }
        } else {
            // AtomDB is not protected. Disregard keychain.
            cout << "[handle_to_metta] path=UNPROTECTED get_atom(handle) key_sensitive="
                 << (key_sensitive_atomdb != nullptr) << endl;
            atom = atomdb->get_atom(handle);
        }

        cout << "[handle_to_metta] get_atom result=" << (atom == nullptr ? "nullptr" : "ok")
             << " handle=" << handle << endl;
        if (atom != nullptr) {
            cout << "[handle_to_metta] atom.type=" << atom->type
                 << " is_node=" << Atom::is_node(*atom) << endl;
            string expression_tag = atom->type;
            if (Atom::is_node(*atom)) {
                if (expression_tag == MettaMapping::SYMBOL_NODE_TYPE) {
                    answer = dynamic_pointer_cast<Node>(atom)->name;
                } else {
                    RAISE_ERROR("Node type \"" + expression_tag + "\" can't be mapped to MeTTa");
                }
            } else {
                if (expression_tag == MettaMapping::EXPRESSION_LINK_TYPE) {
                    vector<string> targets;
                    for (string& _handle : dynamic_pointer_cast<Link>(atom)->targets) {
                        cout << "[handle_to_metta] recurse target=" << _handle
                             << " parent=" << handle << endl;
                        targets.push_back(handle_to_metta_recursion(
                            _handle, mapping, populate_map, keychain, atomdb, key_sensitive_atomdb));
                    }
                    answer = MettaMapping::metta_expr(targets);
                } else {
                    RAISE_ERROR("Link type \"" + expression_tag + "\" can't be mapped to MeTTa");
                }
            }
        } else {
            cout << "[handle_to_metta] UNKNOWN handle=" << handle
                 << " (get_atom returned nullptr; atom may exist but be unauthorized, or missing)"
                 << endl;
            RAISE_ERROR("Unknown handle in handle_to_metta(): " + handle);
        }
        if (populate_map) {
            mapping[handle] = answer;
        }
    }
    return answer;
}
