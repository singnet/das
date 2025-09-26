#include "ImplicationProcessor.h"

#include "AtomDBSingleton.h"
#include "Logger.h"

#define C1 "C1"
#define C2 "C2"

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;

ImplicationProcessor::ImplicationProcessor() {}

LinkSchema ImplicationProcessor::build_pattern_query(const string& handle) {
    // clang-format off
    vector<string> tokens = {
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "EVALUATION",
            "LINK", "Expression", "2",
                "NODE", "Symbol", "PREDICATE",
                "ATOM", handle,
            "LINK_TEMPLATE", "Expression", "2",
                "NODE", "Symbol", "CONCEPT",
                "VARIABLE", "PX"};
    // clang-format on
    return LinkSchema(tokens);
}

static vector<string> build_implication_query(const string& handle){
    // clang-format off
    vector<string> tokens = {
        "OR", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "ATOM", handle,
                "VARIABLE", C1,
            "AND", "2",
                "LINK_TEMPLATE", "Expression", "3",
                    "NODE", "Symbol", "EVALUATION",
                    "ATOM", handle,
                    "VARIABLE", C2,
                "LINK_TEMPLATE", "Expression", "3",
                    "NODE", "Symbol", "EQUIVALENCE",
                    "VARIABLE", C2,
                    "VARIABLE", C1
    };
    // clang-format on
    return tokens;

}

bool ImplicationProcessor::link_exists(const string& handle1, const string& handle2) {
    Node implication_node("Symbol", "IMPLICATION");
    vector<string> targets_p1_p2 = {implication_node.handle(), handle1, handle2};
    vector<string> targets_p2_p1 = {implication_node.handle(), handle2, handle1};
    shared_ptr<Link> p1_link = make_shared<Link>("Expression", targets_p1_p2);
    shared_ptr<Link> p2_link = make_shared<Link>("Expression", targets_p2_p1);
    return AtomDBSingleton::get_instance()->link_exists(p1_link->handle()) &&
           AtomDBSingleton::get_instance()->link_exists(p2_link->handle());
}

vector<shared_ptr<Link>> ImplicationProcessor::process_query(shared_ptr<QueryAnswer> query_answer,
                                                             optional<vector<string>> extra_params) {
    // P1 P2
    if (query_answer->handles.size() < 2) {
        LOG_INFO("Insufficient handles provided, skipping implication processing.");
        return {};
    }
    string p1_handle = query_answer->handles[0];
    string p2_handle = query_answer->handles[1];
    string context = "";
    if (extra_params.has_value()) {
        context = extra_params.value().front();
    }
    if (p1_handle == p2_handle) {
        LOG_INFO("P1 and P2 are the same, skipping implication processing.");
        return {};
    }

    // Check if links is already in DB
    if (link_exists(p1_handle, p2_handle)) {
        LOG_INFO("Link already exists for " << p1_handle << " and " << p2_handle
                                            << ", skipping implication processing.");
        return {};
    }

    LOG_DEBUG("Processing implication query for handles: " << p1_handle << " and " << p2_handle);
    vector<string> p1_query;
    vector<string> p2_query;
    p1_query = build_implication_query(p1_handle);
    p2_query = build_implication_query(p2_handle);
    vector<vector<string>> queries = {p1_query, p2_query};
    vector<double> counts;
    double count_intersection = 0;
    double count_union = 0;
    QueryAnswerElement target_element(C1);
    compute_counts(queries, context, target_element, counts, count_intersection, count_union);
    double p1_set_size = counts[0];
    double p2_set_size = counts[1];

    LOG_DEBUG("INTERSECTION: " << count_intersection);
    LOG_DEBUG("UNION: " << count_union);
    LOG_DEBUG("P1 set size: " << p1_set_size);
    LOG_DEBUG("P2 set size: " << p2_set_size);

    if (p1_set_size == 0 || p2_set_size == 0 || count_intersection == 0) {
        LOG_INFO("No satisfying set found for " << p1_handle << " and " << p2_handle
                                                << ", skipping implication processing.");
        return {};
    }

    double p1_p2_strength = count_intersection / p1_set_size;
    double p2_p1_strength = count_intersection / p2_set_size;

    vector<shared_ptr<Link>> result;
    Node implication_node("Symbol", "IMPLICATION");
    Properties custom_attributes_p1;
    custom_attributes_p1["strength"] = p1_p2_strength;
    custom_attributes_p1["confidence"] = 1;
    Properties custom_attributes_p2;
    custom_attributes_p2["strength"] = p2_p1_strength;
    custom_attributes_p2["confidence"] = 1;
    vector<string> targets_p1_p2 = {implication_node.handle(), p1_handle, p2_handle};
    vector<string> targets_p2_p1 = {implication_node.handle(), p2_handle, p1_handle};
    bool is_toplevel = false;
    shared_ptr<Link> p1_link =
        make_shared<Link>("Expression", targets_p1_p2, is_toplevel, custom_attributes_p1);
    shared_ptr<Link> p2_link =
        make_shared<Link>("Expression", targets_p2_p1, is_toplevel, custom_attributes_p2);

    result.push_back(p1_link);
    result.push_back(p2_link);

    return result;
}
