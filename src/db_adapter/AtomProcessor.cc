#include "AtomProcessor.h"

#include "AtomDBSingleton.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace db_adapter;
using namespace atomdb;
using namespace commons;

AtomProcessor::AtomProcessor() {
    this->atomdb = AtomDBSingleton::get_instance();
    this->set_pattern_index();
}

void AtomProcessor::set_pattern_index() {
    auto db = dynamic_pointer_cast<RedisMongoDB>(AtomDBSingleton::get_instance());

    string tokens = "LINK_TEMPLATE Expression 2 VARIABLE v1 VARIABLE v2";
    vector<vector<string>> index_entries = {{"v1", "*"}, {"*", "v2"}};
    LOG_INFO("Adding pattern index schema for: " + tokens + "...");
    db->add_pattern_index_schema(tokens, index_entries);

    tokens = "LINK_TEMPLATE Expression 3 VARIABLE v1 VARIABLE v2 VARIABLE v3";
    index_entries = {{"v1", "*", "*"},
                     {"v1", "v2", "*"},
                     {"v1", "*", "v3"},
                     {"*", "v2", "*"},
                     {"*", "v2", "v3"},
                     {"*", "*", "v3"}};
    LOG_INFO("Adding pattern index schema for: " + tokens + "...");
    db->add_pattern_index_schema(tokens, index_entries);
}

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
    auto result = this->handle_set.insert(handle);
    return !result.second;
}

void AtomProcessor::process_atoms(vector<Atom*> atoms) {
    if (atoms.empty()) return;

    for (auto& atom : atoms) {
        if (!this->has_handle(atom->handle())) {
            this->atoms.push_back(atom);
        } else {
            delete atom;
        }
    }

    if (this->atoms.size() >= 100000) {
        LOG_INFO("AtomProcessor batch size reached. Persisting " << this->atoms.size()
                                                                 << " atoms to AtomDB...");
        this->atomdb->add_atoms(this->atoms, false, true);
        for (auto& atom : this->atoms) {
            delete atom;
        }
        this->atoms.clear();
        this->handle_set.clear();
    }
}
