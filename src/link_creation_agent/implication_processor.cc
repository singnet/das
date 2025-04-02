#include "implication_processor.h"
#include "AtomDBSingleton.h"
#include "link.h"

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

void ImplicationProcessor::set_das_node(shared_ptr<DASNode> das_node) { this->das_node = das_node; }

void ImplicationProcessor::set_mutex(shared_ptr<mutex> processor_mutex) { this->processor_mutex = processor_mutex; }
vector<string> ImplicationProcessor::get_satisfying_set_query(const string& p1, const string& p2) {
    // clang-format off
    vector<string> pattern_query = {
        "AND", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "PREDICATE",
                    "NODE", "Symbol", p1,
                "VARIABLE", "C",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "PREDICATE",
                    "NODE", "Symbol", p2,
                "VARIABLE", "C"
    };
    // clang-format on
    return pattern_query;
}

vector<vector<string>> ImplicationProcessor::process(
    QueryAnswer* query_answer, std::optional<std::vector<std::string>> extra_params) {
    // P1 P2
    HandlesAnswer* handles_answer = dynamic_cast<HandlesAnswer*>(query_answer);
    string p1_handle = handles_answer->assignment.get("P1");
    string p2_handle = handles_answer->assignment.get("P2");
    if (p1_handle == p2_handle) {
        // clang-format off
        #ifdef DEBUG
        cout << "ImplicationProcessor::process: P1 and P2 are the same, skipping implication processing." << endl;
        #endif
        // clang-format on
        return {};
    }
    string p1_name = AtomDBSingleton::get_instance()->get_atom_document(p1_handle.c_str())->get("name");
    string p2_name = AtomDBSingleton::get_instance()->get_atom_document(p2_handle.c_str())->get("name");
    vector<string> pattern_query_1 = {
        "LINK_TEMPLATE", "Expression", "3",
        "NODE", "Symbol", "EVALUATION",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "PREDICATE",
            "NODE", "Symbol", p1_name,
        "VARIABLE", "P1"
    };
    vector<string> pattern_query_2 = {
        "LINK_TEMPLATE", "Expression", "3",
        "NODE", "Symbol", "EVALUATION",
        "LINK_TEMPLATE", "Expression", "2",
            "NODE", "Symbol", "PREDICATE",
            "NODE", "Symbol", p2_name,
        "VARIABLE", "P2"
    };
    int p1_set_size = this->das_node->count_query(pattern_query_1);
    cout << "ImplicationProcessor::process: p1_set_size(" << p1_name << "): " << p1_set_size << endl;
    int p2_set_size = this->das_node->count_query(pattern_query_2);
    cout << "ImplicationProcessor::process: p2_set_size(" << p2_name << "): " << p2_set_size << endl;
    int p1_p2_set_size = this->das_node->count_query(get_satisfying_set_query(p1_name, p2_name));
    cout << "ImplicationProcessor::process: p1_p2_set_size(" << p1_name << ", " << p2_name << "): " << p1_p2_set_size << endl;
    if (p1_set_size == 0 || p2_set_size == 0 || p1_p2_set_size == 0) {
        // clang-format off
        #ifdef DEBUG
        cout << "ImplicationProcessor::process: No pattern found for " << p1_name << " and " 
        << p2_name << ", skipping implication processing." << endl;
        #endif
        // clang-format on
        return {};
    }
    double p1_p2_strength = double(p1_set_size) / p1_p2_set_size;
    double p2_p1_strength = double(p2_set_size) / p1_p2_set_size;
    
    vector<vector<string>> result;
    Link link_p1_p2;
    link_p1_p2.set_type("IMPLICATION");
    link_p1_p2.add_target(p1_handle);
    link_p1_p2.add_target(p2_handle);
    auto custom_field_p1_p2 = CustomField("truth_value");
    custom_field_p1_p2.add_field("strength", to_string(p1_p2_strength));
    custom_field_p1_p2.add_field("confidence", to_string(1));
    link_p1_p2.add_custom_field(custom_field_p1_p2);

    Link link_p2_p1;
    link_p2_p1.set_type("IMPLICATION");
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