#include "AtomProcessor.h"
#include "AtomDBSingleton.h"


using namespace db_adapter;
using namespace atomdb;
using namespace commons;


AtomProcessor::AtomProcessor() {
    this->atomdb = AtomDBSingleton::get_instance();
}


bool AtomProcessor::has_handle(const string& handle) {
    auto exists = this->handle_trie.exists(handle);
    if (exists) return false;
    this->handle_trie.insert(handle, new EmptyTrieValue());
    return true;
}

void AtomProcessor::process_atoms(vector<Atom*> atoms) {
    if (atoms.empty()) return;

    if (this->atoms.size() >= 10000) {
        this->atomdb->add_atoms(this->atoms, false, true);
        for (auto& atom : this->atoms) {
            delete atom;
        }
        this->atoms.clear();
    } else {
        for (auto& atom : atoms) {
            if (!this->has_handle(atom->handle())) {
                this->atoms.push_back(atom);
            } else {
                delete atom;
            }
        }
    }
}
