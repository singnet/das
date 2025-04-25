#include "equivalence_processor.h"

#include <iostream>

#include "AtomDBSingleton.h"
#include "Logger.h"
#include "PatternMatchingQueryProxy.h"
#include "link.h"
#include "link_creation_console.h"

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;
using namespace atomdb;

EquivalenceProcessor::EquivalenceProcessor() {
    try {
        AtomDBSingleton::init();
    } catch (const std::exception& e) {
        // TODO fix this
    }
}

void EquivalenceProcessor::set_mutex(shared_ptr<mutex> processor_mutex) {
    this->processor_mutex = processor_mutex;
}

// vector<string> EquivalenceProcessor::get_pattern_query(const string& c1, const string& c2) {
//     // clang-format off
//     // vector<string> pattern_query = {
//     //     "OR", "2",
//     //         "LINK_TEMPLATE", "Expression", "3",
//     //             "NODE", "Symbol", "EVALUATION",
//     //             "LINK_TEMPLATE", "Expression", "2",
//     //                 "NODE", "Symbol", "PREDICATE",
//     //                 "VARIABLE", "P",
//     //             "LINK", "Expression", "2",
//     //                 "NODE", "Symbol", "CONCEPT",
//     //                 "NODE", "Symbol", c1,
//     //         "LINK_TEMPLATE", "Expression", "3",
//     //             "NODE", "Symbol", "EVALUATION",
//     //             "LINK_TEMPLATE", "Expression", "2",
//     //                 "NODE", "Symbol", "PREDICATE",
//     //                 "VARIABLE", "P",
//     //             "LINK", "Expression", "2",
//     //                 "NODE", "Symbol", "CONCEPT",
//     //                 "NODE", "Symbol", c2
//     // };

//     string pattern_query_str = "OR 2 "
//                                 "LINK_TEMPLATE Expression 3 "
//                                 "NODE Symbol EVALUATION "
//                                 "LINK_TEMPLATE Expression 2 "
//                                 "NODE Symbol PREDICATE "
//                                 "VARIABLE P "
//                                 "LINK Expression 2 "
//                                 "NODE Symbol CONCEPT ";
//     pattern_query_str += c1 + " ";
//     pattern_query_str += "LINK_TEMPLATE Expression 3 "
//                          "NODE Symbol EVALUATION "
//                          "LINK_TEMPLATE Expression 2 "
//                          "NODE Symbol PREDICATE "
//                          "VARIABLE P "
//                          "LINK Expression 2 "
//                          "NODE Symbol CONCEPT ";
//     pattern_query_str += c2;

//     // clang-format on
//     return Utils::split(pattern_query_str, ' ');
// }

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
    for(const auto& q_template : templates) {
        for (const auto& token : q_template) {
            pattern_query.push_back(token);
        }
    }
    return pattern_query;
}

void EquivalenceProcessor::set_das_node(shared_ptr<service_bus::ServiceBus> das_node) {
    this->das_node = das_node;
}

vector<vector<string>> EquivalenceProcessor::process(
    shared_ptr<QueryAnswer> query_answer, std::optional<std::vector<std::string>> extra_params) {
    // C1 C2
    // HandlesAnswer* handles_answer = dynamic_cast<HandlesAnswer*>(query_answer);
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
    vector<string> c1_name;
    vector<string> c2_name;
    string c1_metta;
    string c2_metta;
    try {
        auto c1_atom = Console::get_instance()->get_atom(c1_handle);
        auto c2_atom = Console::get_instance()->get_atom(c2_handle);
        if (std::holds_alternative<Node>(c1_atom)) {
            c1_name = std::get<Node>(c1_atom).tokenize();
            // c1_name = Utils::join(node_tokens, ' ');
            c1_metta = Console::get_instance()->print_metta(c1_name);
        } else if (std::holds_alternative<shared_ptr<Link>>(c1_atom)) {
            c1_name = std::get<shared_ptr<Link>>(c1_atom)->tokenize(false);
            // c1_name = Utils::join(link_tokens, ' ');
            auto tokenize_metta = std::get<shared_ptr<Link>>(c1_atom)->tokenize();
            c1_metta = Console::get_instance()->print_metta(tokenize_metta);
        }
        if (std::holds_alternative<Node>(c2_atom)) {
            c2_name = std::get<Node>(c2_atom).tokenize();
            // c2_name = Utils::join(node_tokens, ' ');
            c2_metta = Console::get_instance()->print_metta(c2_name);
        } else if (std::holds_alternative<shared_ptr<Link>>(c2_atom)) {
            c2_name = std::get<shared_ptr<Link>>(c2_atom)->tokenize(false);
            // c2_name = Utils::join(link_tokens, ' ');
            auto tokenize_metta = std::get<shared_ptr<Link>>(c2_atom)->tokenize();
            c2_metta = Console::get_instance()->print_metta(tokenize_metta);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get handles: " << c1_handle << ", " << c2_handle);
        LOG_ERROR("Exception: " << e.what());
        return {};
    }

    LOG_DEBUG("(" << c1_metta << ", " << c2_metta << ")");
    auto pattern_query = get_pattern_query(c1_name, c2_name);
    // if (this->das_node == nullptr) {
    //     LOG_ERROR("DASNode is not set");
    // }
    if (this->processor_mutex == nullptr) {
        LOG_ERROR("processor_mutex is not set");
    }
    // this->processor_mutex->lock();
    // LOG_DEBUG("Sending Equivalence Query");
    LOG_DEBUG("Query: " << Utils::join(pattern_query, ' '));
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
            LOG_DEBUG("Found only one pattern for " << c1_metta << " and " << c2_metta
                                                    << ", checking if it is correct...");
            count = count_query(pattern_query, context, false);
            if (count <= 1) {
                LOG_DEBUG("Found only one pattern for " << c1_metta << " and " << c2_metta
                                                        << ", skipping equivalence processing.");
                return {};
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception: " << e.what());
        return {};
    }
    // two elements per query answer
    double strength = double(2) / count;
    LOG_INFO("(" << c1_metta << ", " << c2_metta << ") "
                 << "Strength: " << strength);
    // LOG_DEBUG("2/" << count);
    vector<vector<string>> result;
    Node equivalence_node;
    equivalence_node.type = "Symbol";
    equivalence_node.value = "EQUIVALENCE";
    Link link_c1_c2;
    link_c1_c2.set_type("Expression");
    link_c1_c2.add_target(equivalence_node);
    link_c1_c2.add_target(c1_handle);
    link_c1_c2.add_target(c2_handle);
    auto custom_field = CustomField("truth_value");
    custom_field.add_field("strength", to_string(strength));
    custom_field.add_field("confidence", to_string(1));
    link_c1_c2.add_custom_field(custom_field);

    Link link_c2_c1;
    link_c2_c1.set_type("Expression");
    link_c2_c1.add_target(equivalence_node);
    link_c2_c1.add_target(c2_handle);
    link_c2_c1.add_target(c1_handle);
    auto custom_field2 = CustomField("truth_value");
    custom_field2.add_field("strength", to_string(strength));
    custom_field2.add_field("confidence", to_string(1));
    link_c2_c1.add_custom_field(custom_field2);

    result.push_back(link_c1_c2.tokenize());
    result.push_back(link_c2_c1.tokenize());
    return result;
}