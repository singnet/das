#include "equivalence_processor.h"
#include "HandlesAnswer.h"
#include "link.h"
#include <iostream>

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;


vector<string> EquivalenceProcessor::get_pattern_query(const string& c1, const string& c2) {
    // clang-format off
    vector<string> pattern_query = {
        "OR", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "PREDICATE",
                    "VARIABLE", "P",
                "HANDLE", c1,
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "PREDICATE",
                    "VARIABLE", "P",
                "HANDLE", c2
    };
    // clang-format on
    return pattern_query;
}

void EquivalenceProcessor::set_das_node(shared_ptr<DASNode> das_node) {
    this->das_node = das_node;
}

vector<vector<string>> EquivalenceProcessor::process(
    QueryAnswer* query_answer, 
    std::optional<std::vector<std::string>> extra_params) {
        // TODO
        // C1 C2
        HandlesAnswer* handles_answer = dynamic_cast<HandlesAnswer*>(query_answer);
        string c1_handle = handles_answer->assignment.get("C1");
        string c2_handle = handles_answer->assignment.get("C2");
        auto pattern_query = get_pattern_query(c1_handle, c2_handle);
        if (this->das_node == nullptr) {
            throw runtime_error("DASNode is not set");
        }
        int count = this->das_node->count_query(pattern_query); // TODO context and instantiate das_node
        
        if (count <= 0) {
            return {};
        }
        // two elements per query answer
        int strength = 2 / count;
        vector<vector<string>> result;
        Link link_c1_c2;
        link_c1_c2.set_type("EQUIVALENCE");
        link_c1_c2.add_target(c1_handle);
        link_c1_c2.add_target(c2_handle);
        auto custom_field = CustomField("truth_value");
        custom_field.add_field("strength", to_string(strength));
        custom_field.add_field("confidence", to_string(1));

        Link link_c2_c1;
        link_c2_c1.set_type("EQUIVALENCE");
        link_c2_c1.add_target(c2_handle);
        link_c2_c1.add_target(c1_handle);
        auto custom_field2 = CustomField("truth_value");
        custom_field2.add_field("strength", to_string(strength));
        custom_field2.add_field("confidence", to_string(1));

        result.push_back(link_c1_c2.tokenize());
        result.push_back(link_c2_c1.tokenize());
        return result;
    }