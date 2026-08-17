#include "Link.h"
#include "LinkCreator.h"

using namespace link_creators;

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
        LOG_INFO("ADD LINK: [" + std::to_string(strength) + "] " + new_link->metta_representation(*decoder()));
        db->add_link(new_link.get());
        vector<string> determiners = {handle};
        determiners.insert(determiners.end(), targets.begin(), targets.end());
        add_determiners(determiners);
        AttentionBrokerClient::correlate(set<string>(targets), _context);
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
