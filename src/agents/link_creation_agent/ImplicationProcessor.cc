#include "ImplicationProcessor.h"

#include "Logger.h"

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

LinkSchema ImplicationProcessor::build_satisfying_set_query(const string& p1_handle,
                                                            const string& p2_handle) {
    // clang-format off
    vector<string> tokens = {
        "LINK_TEMPLATE", "AND", "2", 
            "LINK_TEMPLATE", "Expression", "3", 
                "NODE", "Symbol","EVALUATION", 
                "LINK", "Expression", "2", 
                    "NODE", "Symbol", "PREDICATE",
                    "ATOM", p1_handle, 
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "CONCEPT",
                    "VARIABLE", "C",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "PREDICATE",
                    "ATOM", p2_handle,
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "CONCEPT",
                    "VARIABLE", "C"
    };
    // clang-format on
    return LinkSchema(tokens);
}

vector<shared_ptr<Link>> ImplicationProcessor::process_query(shared_ptr<QueryAnswer> query_answer,
                                                             optional<vector<string>> extra_params) {
    // P1 P2
    string p1_handle = query_answer->assignment.get("P1");
    string p2_handle = query_answer->assignment.get("P2");
    string context = "";
    if (extra_params.has_value()) {
        context = extra_params.value().front();
    }
    if (p1_handle == p2_handle) {
        LOG_INFO("P1 and P2 are the same, skipping implication processing.");
        return {};
    }
    LinkSchema p1_schema = build_pattern_query(p1_handle);
    LinkSchema p2_schema = build_pattern_query(p2_handle);

    vector<string> p1_query;
    vector<string> p2_query;
    p1_schema.tokenize(p1_query);
    p2_schema.tokenize(p2_query);

    LOG_DEBUG("Processing implication query for handles: " << p1_handle << " and " << p2_handle);
    int p1_set_size = count_query(p1_query, context);
    LOG_DEBUG("P1 set size: " << p1_set_size);
    int p2_set_size = count_query(p2_query, context);
    LOG_DEBUG("P2 set size: " << p2_set_size);
    LOG_DEBUG("Building satisfying set query for handles: " << p1_handle << " and " << p2_handle);
    auto satisfying_set_query = build_satisfying_set_query(p1_handle, p2_handle).tokenize();
    // remove the first token (LINK_TEMPLATE) to be AND operator
    satisfying_set_query.erase(satisfying_set_query.begin());  // TODO remove when add operators support
    int p1_p2_set_size = count_query(satisfying_set_query, context);
    LOG_DEBUG("P1 and P2 satisfying set size: " << p1_p2_set_size);

    if (p1_set_size == 0 || p2_set_size == 0 || p1_p2_set_size == 0) {
        LOG_INFO("No satisfying set found for " << p1_handle << " and " << p2_handle
                                                << ", skipping implication processing.");
        return {};
    }
    double p1_p2_strength = double(p1_p2_set_size) / double(p1_set_size);
    double p2_p1_strength = double(p1_p2_set_size) / double(p2_set_size);

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
    shared_ptr<Link> p1_link = make_shared<Link>("Expression", targets_p1_p2, custom_attributes_p1);
    shared_ptr<Link> p2_link = make_shared<Link>("Expression", targets_p2_p1, custom_attributes_p2);
    result.push_back(p1_link);
    result.push_back(p2_link);

    return result;
}
