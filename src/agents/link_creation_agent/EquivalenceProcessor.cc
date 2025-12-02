#include "EquivalenceProcessor.h"

#include "Logger.h"

#define P1 "P1"
#define P2 "P2"

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;

EquivalenceProcessor::EquivalenceProcessor() {}

bool EquivalenceProcessor::link_exists(const string& handle1, const string& handle2) {
    Node equivalence_node("Symbol", "Equivalence");
    vector<string> targets_c1_c2 = {equivalence_node.handle(), handle1, handle2};
    vector<string> targets_c2_c1 = {equivalence_node.handle(), handle2, handle1};
    shared_ptr<Link> link_c1_c2 = make_shared<Link>("Expression", targets_c1_c2);
    shared_ptr<Link> link_c2_c1 = make_shared<Link>("Expression", targets_c2_c1);
    return AtomDBSingleton::get_instance()->link_exists(link_c1_c2->handle()) &&
           AtomDBSingleton::get_instance()->link_exists(link_c2_c1->handle());
}

static vector<string> build_equivalence_query(const string& handle) {
    // clang-format off
    vector<string> tokens = {
        "OR", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Evaluation",
                "VARIABLE", P1,
                "ATOM", handle,
            "AND", "2",
                "LINK_TEMPLATE", "Expression", "3",
                    "NODE", "Symbol", "Evaluation",
                    "VARIABLE", P2,
                    "ATOM", handle,
                "LINK_TEMPLATE", "Expression", "3",
                    "NODE", "Symbol", "Implication",
                    "VARIABLE", P2,
                    "VARIABLE", P1
    };
    // clang-format on
    return tokens;
}

vector<shared_ptr<Link>> EquivalenceProcessor::process_query(shared_ptr<QueryAnswer> query_answer,
                                                             optional<vector<string>> extra_params) {
    // C1 C2
    if (query_answer->handles.size() < 2) {
        LOG_INFO("Insufficient handles provided, skipping equivalence processing.");
        return {};
    }
    string c1_handle = query_answer->handles[0];
    string c2_handle = query_answer->handles[1];
    string context = "";
    LOG_DEBUG("Processing equivalence query for handles: " << c1_handle << " and " << c2_handle);
    if (extra_params.has_value()) {
        context = extra_params.value().front();
    }
    if (c1_handle == c2_handle) {
        LOG_INFO("C1 and C2 are the same, skipping equivalence processing.");
        return {};
    }

    vector<string> c1_query;
    vector<string> c2_query;
    c1_query = build_equivalence_query(c1_handle);
    c2_query = build_equivalence_query(c2_handle);
    vector<vector<string>> queries = {c1_query, c2_query};
    vector<double> counts;
    double count_intersection = 0;
    double count_union = 0;
    QueryAnswerElement target_element(P1);
    compute_counts(queries, context, target_element, counts, count_intersection, count_union);

    LOG_DEBUG("INTERSECTION: " << count_intersection);
    LOG_DEBUG("UNION: " << count_union);
    LOG_DEBUG("C1 set size: " << counts[0]);
    LOG_DEBUG("C2 set size: " << counts[1]);

    if (count_intersection == 0 || count_union == 0) {
        LOG_INFO("No intersection or union found for " << c1_handle << " and " << c2_handle
                                                       << ", skipping equivalence processing.");
        return {};
    }

    double strength = count_intersection / count_union;
    vector<shared_ptr<Link>> result;
    Node equivalence_node("Symbol", "Equivalence");
    try {
        AtomDBSingleton::get_instance()->add_node(&equivalence_node);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to add node to AtomDB: " << e.what());
    }
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
