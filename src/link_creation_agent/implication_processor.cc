#include "implication_processor.h"
#include "AtomDBSingleton.h"
#include "PatternMatchingQueryProxy.h"
#include "link.h"
#include "Logger.h"

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;
using namespace atomdb;

ImplicationProcessor::ImplicationProcessor() {
    try {
        AtomDBSingleton::init();
    } catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
}

void ImplicationProcessor::set_das_node(shared_ptr<service_bus::ServiceBus> das_node) { this->das_node = das_node; }

void ImplicationProcessor::set_mutex(shared_ptr<mutex> processor_mutex) { this->processor_mutex = processor_mutex; }

vector<string> ImplicationProcessor::get_satisfying_set_query(const string& p1, const string& p2) {
    // clang-format off
    vector<string> pattern_query = {
        "AND", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "PREDICATE",
                    "NODE", "Symbol", p1,
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "CONCEPT",
                    "VARIABLE", "C",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "PREDICATE",
                    "NODE", "Symbol", p2,
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "CONCEPT",
                    "VARIABLE", "C"
    };
    // clang-format on
    return pattern_query;
}


// static int query_counter(vector<string>& query, mutex& query_mutex, string& context, shared_ptr<service_bus::ServiceBus> service_bus) {
//     lock_guard<mutex> lock(query_mutex);
//     shared_ptr<PatternMatchingQueryProxy> count_query = make_shared<PatternMatchingQueryProxy>(query, context, false, true);
//     service_bus->issue_bus_command(count_query);
//     while(!count_query->finished()) {
//         Utils::sleep();
//     }
//     return count_query->get_count();
// }

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
    string p1_name = AtomDBSingleton::get_instance()->get_atom_document(p1_handle.c_str())->get("name");
    string p2_name = AtomDBSingleton::get_instance()->get_atom_document(p2_handle.c_str())->get("name");
    vector<string> pattern_query_1 = {
        "LINK_TEMPLATE", "Expression", "3",
        "NODE", "Symbol", "EVALUATION",
        "LINK", "Expression", "2",
            "NODE", "Symbol", "PREDICATE",
            "NODE", "Symbol", p1_name,
        "LINK_TEMPLATE", "Expression", "2",
        "NODE", "Symbol", "CONCEPT",
        "VARIABLE", "P1"
    };
    vector<string> pattern_query_2 = {
        "LINK_TEMPLATE", "Expression", "3",
        "NODE", "Symbol", "EVALUATION",
        "LINK", "Expression", "2",
            "NODE", "Symbol", "PREDICATE",
            "NODE", "Symbol", p2_name,
        "LINK_TEMPLATE", "Expression", "2",
        "NODE", "Symbol", "CONCEPT",
        "VARIABLE", "P2"
    };
    LOG_DEBUG("Quering for " << p1_name);
    // shared_ptr<PatternMatchingQueryProxy> count_query_p1 = make_shared<PatternMatchingQueryProxy>(pattern_query_1, context, false, true);
    // this->das_node->issue_bus_command(count_query_p1);
    // while(!count_query_p1->finished()) {
    //     Utils::sleep();
    // }
    // int p1_set_size = count_query_p1->get_count();
    int p1_set_size = count_query(pattern_query_1, *this->processor_mutex, context, this->das_node);



    LOG_DEBUG("P1 set size(" << p1_name << "): " << p1_set_size);
    // shared_ptr<PatternMatchingQueryProxy> count_query_p2 = make_shared<PatternMatchingQueryProxy>(pattern_query_2, context, false, true);
    // this->das_node->issue_bus_command(count_query_p2);
    // while (!count_query_p2->finished())
    // {
    //     Utils::sleep();
    // }
    // int p2_set_size = count_query_p2->get_count();
    // int p2_set_size = query_counter(pattern_query_2, *this->processor_mutex, context, this->das_node);
    int p2_set_size = count_query(pattern_query_2, *this->processor_mutex, context, this->das_node);
    

    LOG_DEBUG("P2 set size(" << p2_name << "): " << p2_set_size);
    auto satisfying_set_query = get_satisfying_set_query(p1_name, p2_name);
    // shared_ptr<PatternMatchingQueryProxy> count_query_p1_p2 = make_shared<PatternMatchingQueryProxy>(satisfying_set_query, context, false, true);
    // this->das_node->issue_bus_command(count_query_p1_p2);
    // while (!count_query_p1_p2->finished())
    // {
    //     Utils::sleep();
    // }
    // int p1_p2_set_size = count_query_p1_p2->get_count();
    int p1_p2_set_size = count_query(satisfying_set_query, *this->processor_mutex, context, this->das_node);

    LOG_DEBUG("P1 and P2 set size(" << p1_name << ", " << p2_name << "): " << p1_p2_set_size);
    if (p1_set_size == 0 || p2_set_size == 0 || p1_p2_set_size == 0) {
        LOG_INFO("No pattern found for " << p1_name << " and " << p2_name << ", skipping implication processing.");
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