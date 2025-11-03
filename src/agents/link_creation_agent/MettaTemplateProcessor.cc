#include "MettaTemplateProcessor.h"

#include "AtomDBSingleton.h"
#include "MettaParser.h"
#include "commons/atoms/MettaParserActions.h"  // Compiler is having issues with relative path here

using namespace std;
// using namespace query_engine;
using namespace atoms;
using namespace link_creation_agent;
using namespace metta;

static string parse_metta_expression(string metta_expression,
                                     unordered_map<string, string>& hash_to_metta,
                                     map<string, string>& assignment_table) {
    auto atomdb = AtomDBSingleton::get_instance();
    string metta_expression_cp = metta_expression;
    for (const auto& assignment_pair : assignment_table) {
        if (hash_to_metta.find(assignment_pair.first) == hash_to_metta.end()) {
            auto atom = atomdb->get_atom(assignment_pair.second);
            if (atom == nullptr) {
                Utils::error("Atom with handle " + assignment_pair.second +
                             " not found in AtomDB while creating missing atoms for link creation.");
                continue;
            }
            hash_to_metta[assignment_pair.second] = atom->metta_representation(*atomdb.get());
        }
        Utils::replace_all(
            metta_expression_cp, "$" + assignment_pair.first, hash_to_metta[assignment_pair.second]);
    }
    return metta_expression_cp;
}

static void create_missing_atoms_in_atomdb(string metta_expression,
                                           shared_ptr<QueryAnswer> query_answer,
                                           unordered_map<string, string>& hash_to_metta) {
    auto atomdb = AtomDBSingleton::get_instance();
    auto pre_parser_actions = make_shared<MettaParserActions>();
    MettaParser pre_parser(metta_expression, pre_parser_actions);
    pre_parser.parse(true);
    // Add missing nodes to AtomDB
    for (const auto& element : pre_parser_actions->handle_to_atom) {
        if (dynamic_pointer_cast<Node>(element.second) != nullptr) {
            try {
                atomdb->add_node(dynamic_pointer_cast<Node>(element.second).get(), false);
                LOG_DEBUG("Node added to AtomDB: " << element.second->to_string());
            } catch (const std::exception& e) {
                // LOG_DEBUG("Error adding node to AtomDB: " << e.what());
            }
        }
    }

    // Add missing links
    for (size_t i = 0; i < pre_parser_actions->metta_expressions.size() - 1; i++) {
        auto metta_expression_cp = pre_parser_actions->metta_expressions[i];
        metta_expression_cp =
            parse_metta_expression(metta_expression_cp, hash_to_metta, query_answer->assignment.table);
        LOG_DEBUG("Processing MeTTa link expression for link creation: " << metta_expression_cp);
        auto p_actions = make_shared<MettaParserActions>();
        MettaParser p_parser(metta_expression_cp, p_actions);
        p_parser.parse(true);
        auto link = p_actions->element_stack.top();
        try {
            atomdb->add_link(dynamic_pointer_cast<Link>(link).get(), false);
            LOG_DEBUG("Link added to AtomDB: " << metta_expression_cp);
        } catch (const std::exception& e) {
            // LOG_DEBUG("Error adding link to AtomDB: " << e.what());
        }
    }
}

vector<shared_ptr<Link>> MettaTemplateProcessor::process_query(shared_ptr<QueryAnswer> query_answer,
                                                               optional<vector<string>> extra_params) {
    vector<shared_ptr<Link>> links;
    if (extra_params == nullopt) {
        Utils::error("Invalid link template");
    }
    auto atomdb = AtomDBSingleton::get_instance();
    for (const auto& param : extra_params.value()) {
        LOG_DEBUG("Processing MeTTa link template: " << param);
        create_missing_atoms_in_atomdb(param, query_answer, hash_to_metta);
        string metta_expression = param;
        LOG_DEBUG("Parsing MeTTa link template: " << metta_expression);
        auto parser_actions = make_shared<MettaParserActions>();
        metta_expression =
            parse_metta_expression(metta_expression, hash_to_metta, query_answer->assignment.table);
        MettaParser parser(metta_expression, parser_actions);
        parser.parse(true);
        if (parser_actions->element_stack.size() == 0) {
            Utils::error("Invalid MeTTa link template. Parser returned an empty stack.");
            continue;
        } else if (parser_actions->element_stack.size() > 1) {
            Utils::error("Invalid MeTTa link template with more than 1 toplevel expressions");
            continue;
        } else {
            auto link = parser_actions->element_stack.top();
            LOG_DEBUG("Parsed MeTTa link template: " << link->to_string());
            links.push_back(dynamic_pointer_cast<Link>(link));
        }
    }
    return links;
}