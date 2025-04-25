#include "equivalence_processor.h"

#include "Logger.h"
#include "link_creation_console.h"

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;
using namespace atomdb;

EquivalenceProcessor::EquivalenceProcessor() {}

vector<string> EquivalenceProcessor::get_pattern_query(const vector<string>& c1,
                                                       const vector<string>& c2) {
    // clang-format off
    vector<vector<string>> templates = {
        {"OR", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "PREDICATE",
                    "VARIABLE", "P",
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "CONCEPT"},
                    c1,
            {"LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "PREDICATE",
                    "VARIABLE", "P",
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "CONCEPT"},
                    c2
        };
    // clang-format on
    vector<string> pattern_query;
    for (const auto& q_template : templates) {
        for (const auto& token : q_template) {
            pattern_query.push_back(token);
        }
    }
    return pattern_query;
}

vector<string> EquivalenceProcessor::get_tokenized_atom(const string& handle) {
    vector<string> tokens;
    try {
        auto atom = Console::get_instance()->get_atom(handle);
        if (holds_alternative<Node>(atom)) {
            return get<Node>(atom).tokenize();
        } else if (holds_alternative<shared_ptr<Link>>(atom)) {
            return get<shared_ptr<Link>>(atom)->tokenize(false);
        }
    } catch (const exception& e) {
        LOG_ERROR("Failed to get handle: " << handle);
        LOG_ERROR("Exception: " << e.what());
        return {};
    }
}

Link EquivalenceProcessor::build_link(const string& link_type,
                                      vector<LinkTargetTypes> targets,
                                      vector<CustomField> custom_fields) {
    Link link;
    link.set_type(link_type);
    for (const auto& target : targets) {
        link.add_target(target);
    }
    for (const auto& custom_field : custom_fields) {
        link.add_custom_field(custom_field);
    }
    return link;
}

vector<vector<string>> EquivalenceProcessor::process(shared_ptr<QueryAnswer> query_answer,
                                                     optional<vector<string>> extra_params) {
    // C1 C2
    string c1_handle = query_answer->assignment.get("C1");
    string c2_handle = query_answer->assignment.get("C2");
    string context = "";
    if (extra_params.has_value()) {
        context = extra_params.value().front();
    }
    if (c1_handle == c2_handle) {
        LOG_INFO("C1 and C2 are the same, skipping equivalence processing.");
        return {};
    }
    vector<string> c1_name = get_tokenized_atom(c1_handle);
    vector<string> c2_name = get_tokenized_atom(c2_handle);
    string c1_metta = Console::get_instance()->tokens_to_metta_string(c1_name, false);
    string c2_metta = Console::get_instance()->tokens_to_metta_string(c2_name, false);
    auto pattern_query = get_pattern_query(c1_name, c2_name);
    int count = 0;
    try {
        count = count_query(pattern_query, context);
        if (count <= 0) {
            LOG_DEBUG("No pattern found for " << c1_metta << " and " << c2_metta
                                              << ", skipping equivalence processing.");
            return {};
        }
        // check if there is only one pattern
        if (count == 1) {
            LOG_DEBUG("Found one pattern for " << c1_metta << " and " << c2_metta
                                               << ", checking if it is correct...");
            count = count_query(pattern_query, context, false);
            if (count <= 1) {
                LOG_DEBUG("Found one pattern for " << c1_metta << " and " << c2_metta
                                                   << ", skipping equivalence processing.");
                return {};
            }
        }
    } catch (const exception& e) {
        LOG_ERROR("Exception: " << e.what());
        return {};
    }
    // two elements per query answer
    double strength = double(2) / count;
    LOG_INFO("(" << c1_metta << ", " << c2_metta << ") "
                 << "Strength: " << strength);
    vector<vector<string>> result;
    // create the link
    Node equivalence_node;
    equivalence_node.type = "Symbol";
    equivalence_node.value = "EQUIVALENCE";
    auto custom_field = CustomField("truth_value");
    custom_field.add_field("strength", to_string(strength));
    custom_field.add_field("confidence", to_string(1));
    Link link_c1_c2 = build_link("Expression", {equivalence_node, c1_handle, c2_handle}, {custom_field});
    Link link_c2_c1 = build_link("Expression", {equivalence_node, c2_handle, c1_handle}, {custom_field});
    result.push_back(link_c1_c2.tokenize());
    result.push_back(link_c2_c1.tokenize());
    return result;
}