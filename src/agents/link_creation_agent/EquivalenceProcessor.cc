#include "EquivalenceProcessor.h"

#include "LinkCreationDBHelper.h"
#include "Logger.h"

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;
using namespace atomdb;

// clang-format off
vector<string> EquivalenceProcessor::count_query_template_1 = {
    "OR", "2",
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "EVALUATION",
            "LINK_TEMPLATE", "Expression", "2",
                "NODE", "Symbol", "PREDICATE",
                "VARIABLE", "P",
            "LINK", "Expression", "2",
                "NODE", "Symbol", "CONCEPT"
};

vector<string> EquivalenceProcessor::count_query_template_2 = {
    "LINK_TEMPLATE", "Expression", "3",
        "NODE", "Symbol", "EVALUATION",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "PREDICATE",
            "VARIABLE", "P",
        "LINK", "Expression", "2",
            "NODE", "Symbol", "CONCEPT"
};
// clang-format on

EquivalenceProcessor::EquivalenceProcessor() {}

vector<string> EquivalenceProcessor::get_pattern_query(const vector<string>& c1,
                                                       const vector<string>& c2) {
    vector<string> query;
    query.reserve(count_query_template_1.size() + c1.size() + count_query_template_2.size() + c2.size());
    query.insert(query.end(), count_query_template_1.begin(), count_query_template_1.end());
    query.insert(query.end(), c1.begin(), c1.end());
    query.insert(query.end(), count_query_template_2.begin(), count_query_template_2.end());
    query.insert(query.end(), c2.begin(), c2.end());

    return query;
}

LinkSchema EquivalenceProcessor::build_pattern_query(const string& handle1,
                                                    const string& handle2) {
    LinkSchema pattern_query("OR", 2);
    pattern_query.stack_node("Symbol", "EVALUATION");
    pattern_query.stack_node("Symbol", "PREDICATE");
    pattern_query.stack_untyped_variable("P");
    pattern_query.stack_link("Expression", 2);
    pattern_query.stack_node("Symbol", "CONCEPT");
    pattern_query.stack_atom(handle1);
    pattern_query.stack_link_schema("Expression", 2);
    pattern_query.stack_link_schema("Expression", 3);
    pattern_query.stack_node("Symbol", "EVALUATION");
    pattern_query.stack_node("Symbol", "PREDICATE");
    pattern_query.stack_untyped_variable("P");
    pattern_query.stack_link("Expression", 2);
    pattern_query.stack_node("Symbol", "CONCEPT");
    pattern_query.stack_atom(handle2);
    pattern_query.stack_link_schema("Expression", 2);
    pattern_query.stack_link_schema("Expression", 3);
    pattern_query.build();
    
    return pattern_query;
}

vector<string> EquivalenceProcessor::get_tokenized_atom(const string& handle) {
    vector<string> tokens;
    try {
        auto atom = LinkCreationDBWrapper::get_atom(handle);
        if (holds_alternative<LCANode>(atom)) {
            return get<LCANode>(atom).tokenize();
        } else if (holds_alternative<shared_ptr<link_creation_agent::LCALink>>(atom)) {
            return get<shared_ptr<link_creation_agent::LCALink>>(atom)->tokenize(false);
        }
    } catch (const exception& e) {
        LOG_ERROR("Failed to get handle: " << handle);
        LOG_ERROR("Exception: " << e.what());
        return {};
    }
    return {};
}

link_creation_agent::LCALink EquivalenceProcessor::build_link(const string& link_type,
                                                              vector<LinkTargetTypes> targets,
                                                              vector<CustomField> custom_fields) {
    link_creation_agent::LCALink link;
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
    string c1_metta = LinkCreationDBWrapper::tokens_to_metta_string(c1_name, false);
    string c2_metta = LinkCreationDBWrapper::tokens_to_metta_string(c2_name, false);
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
    LCANode equivalence_node;
    equivalence_node.type = "Symbol";
    equivalence_node.value = "EQUIVALENCE";
    auto custom_field = CustomField("truth_value");
    custom_field.add_field("strength", to_string(strength));
    custom_field.add_field("confidence", to_string(1));
    link_creation_agent::LCALink link_c1_c2 =
        build_link("Expression", {equivalence_node, c1_handle, c2_handle}, {custom_field});
    link_creation_agent::LCALink link_c2_c1 =
        build_link("Expression", {equivalence_node, c2_handle, c1_handle}, {custom_field});
    result.push_back(link_c1_c2.tokenize());
    result.push_back(link_c2_c1.tokenize());
    return result;
}

vector<shared_ptr<Link>> EquivalenceProcessor::process_query(shared_ptr<QueryAnswer> query_answer,
                                    optional<vector<string>> extra_params) {
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
    LinkSchema pattern_query_schema = build_pattern_query(c1_handle, c2_handle);
    // vector<string> c1_name = get_tokenized_atom(c1_handle);
    // vector<string> c2_name = get_tokenized_atom(c2_handle);
    // string c1_metta = LinkCreationDBWrapper::tokens_to_metta_string(c1_name, false);
    // string c2_metta = LinkCreationDBWrapper::tokens_to_metta_string(c2_name, false);
    auto pattern_query = pattern_query_schema.tokenize();
    // remove the first element (the type)
    pattern_query.erase(pattern_query.begin());
    int count = 0;
    try {
        count = count_query(pattern_query, context);
        if (count <= 0) {
            // LOG_DEBUG("No pattern found for " << c1_metta << " and " << c2_metta
            //                                   << ", skipping equivalence processing.");
            return {};
        }
        // check if there is only one pattern
        if (count == 1) {
            // LOG_DEBUG("Found one pattern for " << c1_metta << " and " << c2_metta
            //                                    << ", checking if it is correct...");
            count = count_query(pattern_query, context, false);
            if (count <= 1) {
                // LOG_DEBUG("Found one pattern for " << c1_metta << " and " << c2_metta
                //                                    << ", skipping equivalence processing.");
                return {};
            }
        }
    } catch (const exception& e) {
        LOG_ERROR("Exception: " << e.what());
        return {};
    }
    // two elements per query answer
    double strength = double(2) / count;
    // LOG_INFO("(" << c1_metta << ", " << c2_metta << ") "
    //              << "Strength: " << strength);
    // vector<vector<string>> result;
    // // create the link
    // LCANode equivalence_node;
    // equivalence_node.type = "Symbol";
    // equivalence_node.value = "EQUIVALENCE";
    // auto custom_field = CustomField("truth_value");
    // custom_field.add_field("strength", to_string(strength));
    // custom_field.add_field("confidence", to_string(1));
    // link_creation_agent::LCALink link_c1_c2 =
    //     build_link("Expression", {equivalence_node, c1_handle, c2_handle}, {custom_field});
    // link_creation_agent::LCALink link_c2_c1 =
    //     build_link("Expression", {equivalence_node, c2_handle, c1_handle}, {custom_field});
    // result.push_back(link_c1_c2.tokenize());
    // result.push_back(link_c2_c1.tokenize());
    // return result;
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
