#include "AtomDBUtils.h"
#include "AtomDBSingleton.h"

using namespace atomdb;

AtomDBUtils::AtomDBUtils() {
}

AtomDBUtils::~AtomDBUtils() {
}

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
