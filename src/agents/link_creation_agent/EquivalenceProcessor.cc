#include "EquivalenceProcessor.h"

#include "Logger.h"

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;

EquivalenceProcessor::EquivalenceProcessor() {}

bool EquivalenceProcessor::link_exists(const string& handle1, const string& handle2) {
    Node equivalence_node("Symbol", "EQUIVALENCE");
    vector<string> targets_c1_c2 = {equivalence_node.handle(), handle1, handle2};
    vector<string> targets_c2_c1 = {equivalence_node.handle(), handle2, handle1};
    shared_ptr<Link> link_c1_c2 = make_shared<Link>("Expression", targets_c1_c2);
    shared_ptr<Link> link_c2_c1 = make_shared<Link>("Expression", targets_c2_c1);
    return AtomDBSingleton::get_instance()->link_exists(link_c1_c2->handle()) &&
           AtomDBSingleton::get_instance()->link_exists(link_c2_c1->handle());
}

vector<shared_ptr<Link>> EquivalenceProcessor::process_query(shared_ptr<QueryAnswer> query_answer,
                                                             optional<vector<string>> extra_params) {
    string c1_handle = query_answer->assignment.get("C1");
    string c2_handle = query_answer->assignment.get("C2");
    string context = "";
    LOG_DEBUG("Processing equivalence query for handles: " << c1_handle << " and " << c2_handle);
    if (extra_params.has_value()) {
        context = extra_params.value().front();
    }
    if (c1_handle == c2_handle) {
        LOG_INFO("C1 and C2 are the same, skipping equivalence processing.");
        return {};
    }
    if (link_exists(c1_handle, c2_handle)) {
        LOG_INFO("Link already exists for " << c1_handle << " and " << c2_handle
                                            << ", skipping equivalence processing.");
        return {};
    }

    LinkSchema pattern_query_schema = build_pattern_union_query(c1_handle, c2_handle);
    LinkSchema satisfying_query_schema = build_pattern_set_query(c1_handle, c2_handle);
    auto pattern_query = pattern_query_schema.tokenize();
    auto satisfying_query = satisfying_query_schema.tokenize();
    // remove the first element (LINK_TEMPLATE)
    pattern_query.erase(pattern_query.begin());        // TODO remove when add operators support
    satisfying_query.erase(satisfying_query.begin());  // TODO remove when add operators support
    int pattern_count = 0;
    int satisfying_count = 0;
    try {
        LOG_INFO("Pattern query: " << Utils::join(pattern_query, ' '));
        pattern_count = count_query(pattern_query, context);
        if (pattern_count <= 0) {
            LOG_DEBUG("No pattern found for " << c1_handle << " and " << c2_handle
                                              << ", skipping equivalence processing.");
            return {};
        }
        LOG_INFO("Satisfying query: " << Utils::join(satisfying_query, ' '));
        satisfying_count = count_query(satisfying_query, context);
        if (satisfying_count <= 0) {
            LOG_DEBUG("No satisfying set found for " << c1_handle << " and " << c2_handle
                                                     << ", skipping equivalence processing.");
            return {};
        }
    } catch (const exception& e) {
        LOG_ERROR("Failed to process equivalence query: " << e.what());
        return {};
    }
    double strength = double(satisfying_count) / double(pattern_count);
    vector<shared_ptr<Link>> result;
    Node equivalence_node("Symbol", "EQUIVALENCE");
    Properties custom_attributes;
    custom_attributes["strength"] = strength;
    custom_attributes["confidence"] = 1;
    vector<string> targets_c1_c2 = {equivalence_node.handle(), c1_handle, c2_handle};
    vector<string> targets_c2_c1 = {equivalence_node.handle(), c2_handle, c1_handle};
    bool is_toplevel = false;
    shared_ptr<Link> link_c1_c2 =
        make_shared<Link>("Expression", targets_c1_c2, is_toplevel, custom_attributes);
    shared_ptr<Link> link_c2_c1 =
        make_shared<Link>("Expression", targets_c2_c1, is_toplevel, custom_attributes);
    result.push_back(link_c1_c2);
    result.push_back(link_c2_c1);
    return result;
}
