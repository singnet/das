#define LOG_LEVEL DEBUG_LEVEL
#include <fstream>

#include "Link.h"
#include "LinkCreator.h"
#include "AttentionBrokerClient.h"

using namespace link_creators;
using namespace attention_broker;

bool LinkCreator::add_or_update_link(const vector<string>& targets,
                                     double strength) {

    STACK_TRACE();
    auto db = atomdb();
    bool new_link_created_flag = false;
    shared_ptr<Link> new_link = make_shared<Link>(Link(EXPRESSION, targets, true, {{STRENGTH_TAG, strength}}));
    LOG_DEBUG("Add or update: " + new_link->to_string());
    string handle = new_link->handle();
    if (db->link_exists(handle)) {
        auto old_link = db->get_atom(handle);
        LOG_DEBUG("Link already exists: " + old_link->to_string() + ". Updating");
        if (strength != old_link->custom_attributes.get_or<double>(STRENGTH_TAG, 1)) {
            // Default merger (NULL) upserts/replaces the existing atom.
            db->add_link(new_link.get());
        }
    } else {
        new_link_created_flag = true;
        LOG_DEBUG("Adding new Link to AtomDB");
        db->add_link(new_link.get());
        LOG_INFO("ADD LINK: [" + std::to_string(strength) + "] " + new_link->metta_representation(*decoder()));
        vector<string> determiners = {handle};
        determiners.insert(determiners.end(), targets.begin(), targets.end());
        add_determiners(determiners);
        AttentionBrokerClient::correlate(set<string>(targets.begin(), targets.end()), _context);
        if (get_log_file() != "") {
            save_link_metta(new_link);
        }
    }
    return new_link_created_flag;
}

string LinkCreator::get_node_name(const string& handle) {
    STACK_TRACE();
    auto node = atomdb()->get_node(handle);
    if (node == nullptr) {
        return "";
    } else {
        return node->name;
    }
}

double LinkCreator::get_strength(const string& handle) {
    STACK_TRACE();
    auto atom = atomdb()->get_atom(handle);
    return atom->custom_attributes.get_or<double>(STRENGTH_TAG, 1.0);
}

void LinkCreator::save_link_metta(shared_ptr<Link> link) {
    STACK_TRACE();
    ofstream file;
    file.open(get_log_file(), ios::app);
    if (file.is_open()) {
        file << link->custom_attributes.get_or<double>("strength", 1.0) << ","
             << link->metta_representation(*decoder()) << endl;
        file.close();
    } else {
        RAISE_ERROR("Couldn't open file for writing: " + get_log_file());
    }
}
