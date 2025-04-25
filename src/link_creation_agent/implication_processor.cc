#include "implication_processor.h"

#include "AtomDBSingleton.h"
#include "Logger.h"
#include "PatternMatchingQueryProxy.h"
#include "link.h"
#include "link_creation_console.h"

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;
using namespace atomdb;

ImplicationProcessor::ImplicationProcessor() {
    try {
        AtomDBSingleton::init();
    } catch (const std::exception& e) {
        // TODO fix this
    }
}

void ImplicationProcessor::set_das_node(shared_ptr<service_bus::ServiceBus> das_node) {
    this->das_node = das_node;
}

void ImplicationProcessor::set_mutex(shared_ptr<mutex> processor_mutex) {
    this->processor_mutex = processor_mutex;
}

// vector<string> ImplicationProcessor::get_satisfying_set_query(const string& p1, const string& p2) {
//     // clang-format off
//     vector<string> pattern_query = {
//         "AND", "2",
//             "LINK_TEMPLATE", "Expression", "3",
//                 "NODE", "Symbol", "EVALUATION",
//                 "LINK", "Expression", "2",
//                     "NODE", "Symbol", "PREDICATE",
//                     "NODE", "Symbol", p1,
//                 "LINK_TEMPLATE", "Expression", "2",
//                     "NODE", "Symbol", "CONCEPT",
//                     "VARIABLE", "C",
//             "LINK_TEMPLATE", "Expression", "3",
//                 "NODE", "Symbol", "EVALUATION",
//                 "LINK", "Expression", "2",
//                     "NODE", "Symbol", "PREDICATE",
//                     "NODE", "Symbol", p2,
//                 "LINK_TEMPLATE", "Expression", "2",
//                     "NODE", "Symbol", "CONCEPT",
//                     "VARIABLE", "C"
//     };
//     // clang-format on
//     string pattern_query_str =
//         "AND 2 "
//         "LINK_TEMPLATE Expression 3 "
//         "NODE Symbol EVALUATION "
//         "LINK Expression 2 "
//         "NODE Symbol PREDICATE " +
//         p1 +
//         " "
//         "LINK_TEMPLATE Expression 2 "
//         "NODE Symbol CONCEPT "
//         "VARIABLE C "
//         "LINK_TEMPLATE Expression 3 "
//         "NODE Symbol EVALUATION "
//         "LINK Expression 2 "
//         "NODE Symbol PREDICATE " +
//         p2 +
//         " "
//         "LINK_TEMPLATE Expression 2 "
//         "NODE Symbol CONCEPT "
//         "VARIABLE C";
//     return Utils::split(pattern_query_str, ' ');
// }

vector<string> ImplicationProcessor::get_pattern_query(const vector<string>& p) {
    // clang-format off
    vector<vector<string>> templates = {
        {
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "EVALUATION",    
            "LINK", "Expression", "2",      
                "NODE", "Symbol", "PREDICATE"},
                p,
           {"LINK_TEMPLATE", "Expression", "2",          
                "NODE", "Symbol", "CONCEPT",       
                "VARIABLE", "PX"}
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

vector<string> ImplicationProcessor::get_satisfying_set_query(const vector<string>& p1,
                                                                const vector<string>& p2) {
    // clang-format off
    vector<vector<string>> templates = {
        {"AND", "2",
                "LINK_TEMPLATE", "Expression", "3",
                    "NODE", "Symbol", "EVALUATION",
                    "LINK", "Expression", "2",
                        "NODE", "Symbol", "PREDICATE"},
                        p1,
                   {"LINK_TEMPLATE", "Expression", "2",
                        "NODE", "Symbol", "CONCEPT",
                        "VARIABLE", "C",
                "LINK_TEMPLATE", "Expression", "3",
                    "NODE", "Symbol", "EVALUATION",
                    "LINK", "Expression", "2",
                        "NODE", "Symbol", "PREDICATE"},
                        p2,
                    {"LINK_TEMPLATE", "Expression", "2",
                        "NODE", "Symbol", "CONCEPT",
                        "VARIABLE", "C"}
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

vector<vector<string>> ImplicationProcessor::process(
    shared_ptr<QueryAnswer> query_answer, std::optional<std::vector<std::string>> extra_params) {
    // P1 P2
    // HandlesAnswer* handles_answer = dynamic_cast<HandlesAnswer*>(query_answer);
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
    vector<string> p1_name;
    vector<string> p2_name;
    string p1_metta;
    string p2_metta;
    try {
        auto p1_atom = Console::get_instance()->get_atom(p1_handle);
        auto p2_atom = Console::get_instance()->get_atom(p2_handle);
        if (std::holds_alternative<Node>(p1_atom)) {
            p1_name = std::get<Node>(p1_atom).tokenize();
            // p1_name = Utils::join(node_tokens, ' ');
            p1_metta = Console::get_instance()->print_metta(p1_name);
        } else if (std::holds_alternative<shared_ptr<Link>>(p1_atom)) {
            p1_name = std::get<shared_ptr<Link>>(p1_atom)->tokenize(false);
            // p1_name = Utils::join(link_tokens, ' ');
            auto tokenize_metta = std::get<shared_ptr<Link>>(p1_atom)->tokenize();
            p1_metta = Console::get_instance()->print_metta(tokenize_metta);
        }
        if (std::holds_alternative<Node>(p2_atom)) {
            p2_name = std::get<Node>(p2_atom).tokenize();
            // p2_name = Utils::join(node_tokens, ' ');
            p2_metta = Console::get_instance()->print_metta(p2_name);
        } else if (std::holds_alternative<shared_ptr<Link>>(p2_atom)) {
            p2_name = std::get<shared_ptr<Link>>(p2_atom)->tokenize(false);
            // p2_name = Utils::join(link_tokens, ' ');
            auto tokenize_metta = std::get<shared_ptr<Link>>(p2_atom)->tokenize();
            p2_metta = Console::get_instance()->print_metta(tokenize_metta);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get handles: " << p1_handle << ", " << p2_handle);
        LOG_ERROR("Failed to get atom name: " << e.what());
        return {};
    }

    // clang-format off
    // vector<string> pattern_query_1 = {
    //     "LINK_TEMPLATE", "Expression", "3",
    //         "NODE", "Symbol", "EVALUATION",    
    //         "LINK", "Expression", "2",      
    //             "NODE", "Symbol", "PREDICATE",  
    //             "NODE", "Symbol", p1_name,
    //         "LINK_TEMPLATE", "Expression", "2",          
    //             "NODE", "Symbol", "CONCEPT",       
    //             "VARIABLE", "P1"};
    // vector<string> pattern_query_2 = {"LINK_TEMPLATE", "Expression", "3",          "NODE",   "Symbol",
    //                                   "EVALUATION",    "LINK",       "Expression", "2",      "NODE",
    //                                   "Symbol",        "PREDICATE",  "NODE",       "Symbol", p2_name,
    //                                   "LINK_TEMPLATE", "Expression", "2",          "NODE",   "Symbol",
    //                                   "CONCEPT",       "VARIABLE",   "P2"};

    // clang-format on

    // auto pattern_query_1 = Utils::split(pattern_query_1_str, ' ');
    // auto pattern_query_2 = Utils::split(pattern_query_2_str, ' ');
    // LOG_DEBUG("P1 Query: " << pattern_query_1_str);
    auto pattern_query_1 = get_pattern_query(p1_name);
    auto pattern_query_2 = get_pattern_query(p2_name);
    LOG_DEBUG("P1 Query: " << Utils::join(pattern_query_1, ' '));
    LOG_DEBUG("P2 Query: " << Utils::join(pattern_query_2, ' '));
    int p1_set_size = count_query(pattern_query_1, context);
    LOG_DEBUG("P1 set size(" << p1_metta << "): " << p1_set_size);

    // LOG_DEBUG("P2 Query: " << pattern_query_2_str);
    int p2_set_size = count_query(pattern_query_2, context);
    LOG_DEBUG("P2 set size(" << p2_metta << "): " << p2_set_size);
    auto satisfying_set_query = get_satisfying_set_query(p1_name, p2_name);
    LOG_DEBUG("Satisfying Set Query: " << Utils::join(satisfying_set_query, ' '));
    int p1_p2_set_size = count_query(satisfying_set_query, context);

    LOG_DEBUG("P1 and P2 set size(" << p1_metta << ", " << p2_metta << "): " << p1_p2_set_size);

    if (p1_set_size == 0 || p2_set_size == 0 || p1_p2_set_size == 0) {
        LOG_INFO("No pattern found for " << p1_metta << " and " << p2_metta
                                         << ", skipping implication processing.");
        return {};
    }
    double p1_p2_strength = double(p1_p2_set_size) / double(p1_set_size);
    double p2_p1_strength = double(p1_p2_set_size) / double(p2_set_size);

    vector<vector<string>> result;
    Node implication_node;
    implication_node.type = "Symbol";
    implication_node.value = "IMPLICATION";
    Link link_p1_p2;
    link_p1_p2.set_type("Expression");
    link_p1_p2.add_target(implication_node);
    link_p1_p2.add_target(p1_handle);
    link_p1_p2.add_target(p2_handle);
    auto custom_field_p1_p2 = CustomField("truth_value");
    custom_field_p1_p2.add_field("strength", to_string(p1_p2_strength));
    custom_field_p1_p2.add_field("confidence", to_string(1));
    link_p1_p2.add_custom_field(custom_field_p1_p2);

    Link link_p2_p1;
    link_p2_p1.set_type("Expression");
    link_p2_p1.add_target(implication_node);
    link_p2_p1.add_target(p2_handle);
    link_p2_p1.add_target(p1_handle);
    auto custom_field_p2_p1 = CustomField("truth_value");
    custom_field_p2_p1.add_field("strength", to_string(p2_p1_strength));
    custom_field_p2_p1.add_field("confidence", to_string(1));
    link_p2_p1.add_custom_field(custom_field_p2_p1);

    result.push_back(link_p1_p2.tokenize());
    result.push_back(link_p2_p1.tokenize());

    return result;
}