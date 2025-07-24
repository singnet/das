#include "EquivalenceProcessor.h"

#include "Logger.h"

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;

EquivalenceProcessor::EquivalenceProcessor() {}

LinkSchema EquivalenceProcessor::build_pattern_query(const string& handle1, const string& handle2) {
    // clang-format off
    vector<string> tokens = {
        "LINK_TEMPLATE", "OR", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "PREDICATE",
                    "VARIABLE", "P",
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "CONCEPT",
                    "ATOM", handle1,
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "PREDICATE",
                    "VARIABLE", "P",
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "CONCEPT",
                    "ATOM", handle2
                    
    };
    // clang-format on
    return LinkSchema(tokens);
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
    LinkSchema pattern_query_schema = build_pattern_query(c1_handle, c2_handle);
    auto pattern_query = pattern_query_schema.tokenize();
    // remove the first element (LINK_TEMPLATE)
    pattern_query.erase(pattern_query.begin());  // TODO remove when add operators support
    int count = 0;
    try {
        LOG_INFO("Pattern query: " << Utils::join(pattern_query, ' '));
        count = count_query(pattern_query, context);
        if (count <= 0) {
            LOG_DEBUG("No pattern found for " << c1_handle << " and " << c2_handle
                                              << ", skipping equivalence processing.");
            return {};
        }
        // check if there is only one pattern
        if (count == 1) {
            LOG_DEBUG("Found one pattern for " << c1_handle << " and " << c2_handle
                                               << ", checking if it is correct...");
            count = count_query(pattern_query, context, false);
            if (count <= 1) {
                LOG_DEBUG("Found one pattern for " << c1_handle << " and " << c2_handle
                                                   << ", skipping equivalence processing.");
                return {};
            }
        }
    } catch (const exception& e) {
        LOG_ERROR("Failed to process equivalence query: " << e.what());
        return {};
    }
    // two elements per query answer
    double strength = double(2) / count;
    vector<shared_ptr<Link>> result;
    Node equivalence_node("Symbol", "EQUIVALENCE");
    Properties custom_attributes;
    custom_attributes["strength"] = strength;
    custom_attributes["confidence"] = 1;
    vector<string> targets_c1_c2 = {equivalence_node.handle(), c1_handle, c2_handle};
    vector<string> targets_c2_c1 = {equivalence_node.handle(), c2_handle, c1_handle};
    shared_ptr<Link> link_c1_c2 = make_shared<Link>("Expression", targets_c1_c2, custom_attributes);
    shared_ptr<Link> link_c2_c1 = make_shared<Link>("Expression", targets_c2_c1, custom_attributes);
    result.push_back(link_c1_c2);
    result.push_back(link_c2_c1);
    return result;
}
