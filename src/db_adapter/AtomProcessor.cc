#include "AtomProcessor.h"

#include "AtomDBSingleton.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace db_adapter;
using namespace atomdb;
using namespace commons;

AtomProcessor::AtomProcessor() { this->atomdb = AtomDBSingleton::get_instance(); }

AtomProcessor::~AtomProcessor() {
    if (!this->atoms.empty()) {
        LOG_INFO("AtomProcessor destructor called with "
                 << this->atoms.size() << " unprocessed atoms. Persisting to AtomDB...");
        this->atomdb->add_atoms(this->atoms, false, true);
        for (auto& atom : this->atoms) {
            delete atom;
        }
        this->atoms.clear();
    } else {
        LOG_INFO("AtomProcessor destructor called with no unprocessed atoms.");
    }
}

bool AtomProcessor::has_handle(const string& handle) {
    auto exists = this->handle_trie.exists(handle);
    if (exists) return true;
    this->handle_trie.insert(handle, new EmptyTrieValue());
    return false;
}

void AtomProcessor::process_atoms(vector<Atom*> atoms) {
    if (atoms.empty()) return;

    LOG_INFO("AtomProcessor batch size: " << this->atoms.size() << ". Adding atoms to batch...");

    for (auto& atom : atoms) {
        if (!this->has_handle(atom->handle())) {
            this->atoms.push_back(atom);
        } else {
            delete atom;
        }
    }

    if (this->atoms.size() >= 10000) {
        LOG_INFO("AtomProcessor batch size reached. Persisting " << this->atoms.size()
                                                                 << " atoms to AtomDB...");
        this->atomdb->add_atoms(this->atoms, false, true);
        for (auto& atom : this->atoms) {
            delete atom;
        }
        this->atoms.clear();
    }
}
