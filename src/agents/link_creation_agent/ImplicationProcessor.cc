#include "ImplicationProcessor.h"

#include "LinkCreationDBHelper.h"
#include "Logger.h"

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;
using namespace atomdb;

// clang-format off
vector<string> ImplicationProcessor::pattern_template_1 = {
    "LINK_TEMPLATE", "Expression", "3",
        "NODE", "Symbol", "EVALUATION",    
        "LINK", "Expression", "2",      
            "NODE", "Symbol", "PREDICATE"
};

vector<string> ImplicationProcessor::pattern_template_2 = {
    "LINK_TEMPLATE", "Expression", "2",          
        "NODE", "Symbol", "CONCEPT",       
        "VARIABLE", "PX"
};

vector<string> ImplicationProcessor::ss_template_1 = {
    "AND", "2",
    "LINK_TEMPLATE", "Expression", "3",
        "NODE", "Symbol", "EVALUATION",
        "LINK", "Expression", "2",
            "NODE", "Symbol", "PREDICATE"
};

vector<string> ImplicationProcessor::ss_template_2 = {
    "LINK_TEMPLATE", "Expression", "2",
        "NODE", "Symbol", "CONCEPT",
        "VARIABLE", "C",
    "LINK_TEMPLATE", "Expression", "3",
        "NODE", "Symbol", "EVALUATION",
        "LINK", "Expression", "2",
            "NODE", "Symbol", "PREDICATE"
};

vector<string> ImplicationProcessor::ss_template_3 = {
    "LINK_TEMPLATE", "Expression", "2",
        "NODE", "Symbol", "CONCEPT",
        "VARIABLE", "C"
};

// clang-format on

ImplicationProcessor::ImplicationProcessor() {}

vector<string> ImplicationProcessor::get_pattern_query(const vector<string>& p) {
    vector<string> pattern_query;
    pattern_query.reserve(pattern_template_1.size() + p.size() + pattern_template_2.size());
    pattern_query.insert(pattern_query.end(), pattern_template_1.begin(), pattern_template_1.end());
    pattern_query.insert(pattern_query.end(), p.begin(), p.end());
    pattern_query.insert(pattern_query.end(), pattern_template_2.begin(), pattern_template_2.end());

    return pattern_query;
}

LinkSchema ImplicationProcessor::build_pattern_query(const string& handle) {
    LinkSchema pattern_query("Expression", 3);
    pattern_query.stack_node("Symbol", "EVALUATION");
    pattern_query.stack_node("Symbol", "PREDICATE");
    pattern_query.stack_atom(handle);
    pattern_query.stack_link("Expression", 2);
    pattern_query.stack_node("Symbol", "CONCEPT");
    pattern_query.stack_untyped_variable("PX");
    pattern_query.stack_link_schema("Expression", 2);
    pattern_query.build();
    return pattern_query;
}


LinkSchema ImplicationProcessor::build_satisfying_set_query(const string& p1_handle,
                                                            const string& p2_handle) {
    LinkSchema ss_query("AND", 2); // TODO use AND operator
        ss_query.stack_node("Symbol", "EVALUATION");
        ss_query.stack_node("Symbol", "PREDICATE");
        ss_query.stack_atom(p1_handle);
        ss_query.stack_link("Expression", 2);
        ss_query.stack_node("Symbol", "CONCEPT");
        ss_query.stack_untyped_variable("C");
        ss_query.stack_link_schema("Expression", 2);
        ss_query.stack_link_schema("Expression", 3);
        ss_query.stack_node("Symbol", "EVALUATION");
        ss_query.stack_node("Symbol", "PREDICATE");
        ss_query.stack_atom(p2_handle);
        ss_query.stack_link("Expression", 2);
        ss_query.stack_node("Symbol", "CONCEPT");
        ss_query.stack_untyped_variable("C");
        ss_query.stack_link_schema("Expression", 2);
        ss_query.stack_link_schema("Expression", 3);
    ss_query.build();
    return ss_query;
}


vector<string> ImplicationProcessor::get_satisfying_set_query(const vector<string>& p1,
                                                              const vector<string>& p2) {
    vector<string> pattern_query;
    pattern_query.reserve(ss_template_1.size() + p1.size() + ss_template_2.size() + p2.size() +
                          ss_template_3.size());
    pattern_query.insert(pattern_query.end(), ss_template_1.begin(), ss_template_1.end());
    pattern_query.insert(pattern_query.end(), p1.begin(), p1.end());
    pattern_query.insert(pattern_query.end(), ss_template_2.begin(), ss_template_2.end());
    pattern_query.insert(pattern_query.end(), p2.begin(), p2.end());
    pattern_query.insert(pattern_query.end(), ss_template_3.begin(), ss_template_3.end());
    return pattern_query;
}

vector<string> ImplicationProcessor::get_tokenized_atom(const string& handle) {
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

link_creation_agent::LCALink ImplicationProcessor::build_link(const string& link_type,
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

vector<vector<string>> ImplicationProcessor::process(shared_ptr<QueryAnswer> query_answer,
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
    // vector<string> p1_name = get_tokenized_atom(p1_handle);
    // vector<string> p2_name = get_tokenized_atom(p2_handle);
    // string p1_metta = LinkCreationDBWrapper::tokens_to_metta_string(p1_name, false);
    // string p2_metta = LinkCreationDBWrapper::tokens_to_metta_string(p2_name, false);

    auto pattern_query_1 = get_pattern_query(p1_schema.tokenize());
    auto pattern_query_2 = get_pattern_query(p2_schema.tokenize());
    
    int p1_set_size = count_query(pattern_query_1, context);
    // LOG_DEBUG("P1 set size(" << p1_metta << "): " << p1_set_size);
    int p2_set_size = count_query(pattern_query_2, context);
    // LOG_DEBUG("P2 set size(" << p2_metta << "): " << p2_set_size);
    // auto satisfying_set_query = get_satisfying_set_query(p1_name, p2_name);
    auto satisfying_set_query = build_satisfying_set_query(p1_handle, p2_handle).tokenize();
    // remove the first token to be AND operator
    satisfying_set_query.erase(satisfying_set_query.begin());
    int p1_p2_set_size = count_query(satisfying_set_query, context);
    // LOG_DEBUG("P1 and P2 set size(" << p1_metta << ", " << p2_metta << "): " << p1_p2_set_size);

    if (p1_set_size == 0 || p2_set_size == 0 || p1_p2_set_size == 0) {
        // LOG_INFO("No pattern found for " << p1_metta << " and " << p2_metta
        //                                  << ", skipping implication processing.");
        return {};
    }
    double p1_p2_strength = double(p1_p2_set_size) / double(p1_set_size);
    double p2_p1_strength = double(p1_p2_set_size) / double(p2_set_size);



    vector<vector<string>> result;
    Node implication_node("Symbol", "IMPLICATION");
    Properties custom_attributes_p1;
    custom_attributes_p1["strength"] = to_string(p1_p2_strength);
    custom_attributes_p1["confidence"] = "1";
    Properties custom_attributes_p2;
    custom_attributes_p2["strength"] = to_string(p2_p1_strength);
    custom_attributes_p2["confidence"] = "1";
    Link p1_link("Expression", {implication_node.handle(), p1_handle, p2_handle}, custom_attributes_p1);
    Link p2_link("Expression", {implication_node.handle(), p2_handle, p1_handle}, custom_attributes_p2);
    // LCANode implication_node;
    // implication_node.type = "Symbol";
    // implication_node.value = "IMPLICATION";
    // auto custom_field_p1_p2 = CustomField("truth_value");
    // custom_field_p1_p2.add_field("strength", to_string(p1_p2_strength));
    // custom_field_p1_p2.add_field("confidence", to_string(1));
    // link_creation_agent::LCALink link_p1_p2 =
    //     build_link("Expression", {implication_node, p1_handle, p2_handle}, {custom_field_p1_p2});

    // auto custom_field_p2_p1 = CustomField("truth_value");
    // custom_field_p2_p1.add_field("strength", to_string(p2_p1_strength));
    // custom_field_p2_p1.add_field("confidence", to_string(1));
    // link_creation_agent::LCALink link_p2_p1 =
    //     build_link("Expression", {implication_node, p2_handle, p1_handle}, {custom_field_p2_p1});

    // result.push_back(p1_link.tokenize());
    // result.push_back(p2_link.tokenize());

    return result;
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

    auto pattern_query_1 = get_pattern_query(p1_schema.tokenize());
    auto pattern_query_2 = get_pattern_query(p2_schema.tokenize());
    
    int p1_set_size = count_query(pattern_query_1, context);
    // LOG_DEBUG("P1 set size(" << p1_metta << "): " << p1_set_size);
    int p2_set_size = count_query(pattern_query_2, context);
    // LOG_DEBUG("P2 set size(" << p2_metta << "): " << p2_set_size);
    // auto satisfying_set_query = get_satisfying_set_query(p1_name, p2_name);
    auto satisfying_set_query = build_satisfying_set_query(p1_handle, p2_handle).tokenize();
    // remove the first token to be AND operator
    satisfying_set_query.erase(satisfying_set_query.begin());
    int p1_p2_set_size = count_query(satisfying_set_query, context);
    // LOG_DEBUG("P1 and P2 set size(" << p1_metta << ", " << p2_metta << "): " << p1_p2_set_size);

    if (p1_set_size == 0 || p2_set_size == 0 || p1_p2_set_size == 0) {
        // LOG_INFO("No pattern found for " << p1_metta << " and " << p2_metta
        //                                  << ", skipping implication processing.");
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
