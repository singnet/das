#include "AtomDBUtils.h"

#include "Logger.h"
#include "ProtectedAtomDB.h"
#include "AtomDBSingleton.h"
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

string AtomDBUtils::handle_to_metta(const string& handle, shared_ptr<Keychain> keychain) {
    map<string, string> not_used;
    return handle_to_metta_recursion(handle, not_used, false, keychain);
}

string AtomDBUtils::handle_to_metta(const string& handle, map<string, string>& mapping, shared_ptr<Keychain> keychain) {
    return handle_to_metta_recursion(handle, mapping, true, keychain);
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

string AtomDBUtils::handle_to_metta_recursion(const string& handle, map<string, string>& mapping, bool populate_map, shared_ptr<Keychain> keychain) {

    string answer = "UNKNOWN_HANDLE";
    auto iterator = mapping.find(handle);
    if (iterator != mapping.end()) {
        answer = iterator->second;
    } else {
        shared_ptr<Atom> atom = nullptr;
        shared_ptr<AtomDB> atomdb = AtomDBSingleton::get_instance();
        shared_ptr<ProtectedAtomDB> protected_atomdb = dynamic_pointer_cast<ProtectedAtomDB>(atomdb);

        if (protected_atomdb == nullptr) {
            // AtomDB is not protected. Disregard keychain.
            atom = atomdb->get_atom(handle);
        } else {
            // AtomDB is protected. Keychain must be forwarded.
            if (keychain != nullptr) {
                // TODO __AUTH__ uncomment line below
                //atom = protected_atomdb->get_atom(handle, keychain);
            } else {
                RAISE_ERROR("AtomDB is protected and requires a keychain");
            }
        }

        if (atom != nullptr) {
            if (Atom::is_node(*atom)) {
                answer = dynamic_pointer_cast<Node>(atom)->name;
            } else {
                vector<string> targets;
                for (string& handle : dynamic_pointer_cast<Link>(atom)->targets) {
                    targets.push_back(handle_to_metta_recursion(handle, mapping, populate_map, keychain));
                }
                answer = MettaMapping::metta_expr(targets);
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
