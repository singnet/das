#include "Link.h"
#include "LinkCreator.h"

using namespace link_creators;

shared_ptr<Link> LinkCreator::add_or_update_link(const vector<string>& targets,
                                                 double strength,
                                                 const string& context,
                                                 bool& new_link_created_flag) {

    STACK_TRACE();
    new_link_created_flag = false;
    if (strength < this->_strength_threshold) {
        return nullptr;
    }
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
        if (WRITE_CREATED_LINKS_TO_DB) {
            LOG_DEBUG("Adding new Link to AtomDB");
            LOG_INFO("ADD LINK: [" + std::to_string(strength) + "] " + new_link->metta_representation(*decoder()));
            db->add_link(new_link.get());
            buffer_determiners.push_back({handle, target1, target2});
            AttentionBrokerClient::correlate(set<string>({target1, target2}), context);
        }
        if (WRITE_CREATED_LINKS_TO_FILE) {
            LOG_DEBUG("Writing Link to file: " + PRESET_LINKS_FILE);
            save_link_metta(new_link);
        }
    }
    LOG_LOCAL_DEBUG("Returning from add_or_update_link()");
    return new_link;
}

