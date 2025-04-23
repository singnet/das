#include "equivalence_processor.h"

#include <iostream>

#include "AtomDBSingleton.h"
#include "Logger.h"
#include "PatternMatchingQueryProxy.h"
#include "link.h"

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;
using namespace atomdb;

EquivalenceProcessor::EquivalenceProcessor() {
    try {
        AtomDBSingleton::init();
    } catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
}

void EquivalenceProcessor::set_mutex(shared_ptr<mutex> processor_mutex) {
    this->processor_mutex = processor_mutex;
}

vector<string> EquivalenceProcessor::get_pattern_query(const string& c1, const string& c2) {
    // clang-format off
    vector<string> pattern_query = {
        "OR", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "PREDICATE",
                    "VARIABLE", "P",
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "CONCEPT",
                    "NODE", "Symbol", c1,
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "EVALUATION",
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "PREDICATE",
                    "VARIABLE", "P",
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "CONCEPT",
                    "NODE", "Symbol", c2
    };
    // clang-format on
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
    string c1_name = AtomDBSingleton::get_instance()->get_atom_document(c1_handle.c_str())->get("name");
    string c2_name = AtomDBSingleton::get_instance()->get_atom_document(c2_handle.c_str())->get("name");
    LOG_DEBUG("(" << c1_name << ", " << c2_name << ")");
    auto pattern_query = get_pattern_query(c1_name, c2_name);
    if (this->das_node == nullptr) {
        LOG_ERROR("DASNode is not set");
    }
    if (this->processor_mutex == nullptr) {
        LOG_ERROR("processor_mutex is not set");
    }
    // this->processor_mutex->lock();
    // LOG_DEBUG("Sending Equivalence Query");
    LOG_DEBUG("Query: " << Utils::join(pattern_query, ' '));
    int count = 0;
    try {
        // shared_ptr<PatternMatchingQueryProxy> count_query =
        // make_shared<PatternMatchingQueryProxy>(pattern_query, context, false, true);
        // this->das_node->issue_bus_command(count_query);
        // while (!count_query->finished()) {
        //     // LOG_DEBUG("Waiting for count " << c1_name << " and " << c2_name << " query to finish");
        //     Utils::sleep();
        // }
        // count = count_query->get_count();
        count = count_query(pattern_query, *this->processor_mutex, context, this->das_node);
        // int count = this->das_node->count_query(pattern_query, context, false, 10);  // TODO context
        // this->processor_mutex->unlock();
        if (count <= 0) {
            LOG_DEBUG("No pattern found for " << c1_name << " and " << c2_name
                                              << ", skipping equivalence processing.");
            return {};
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception: " << e.what());
        return {};
    }
    // two elements per query answer
    double strength = double(2) / count;
    LOG_INFO("(" << c1_name << ", " << c2_name << ") "
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