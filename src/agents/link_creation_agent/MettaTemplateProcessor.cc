#include "MettaTemplateProcessor.h"

#include "AtomDBSingleton.h"
#include "MettaParser.h"
#include "commons/atoms/MettaParserActions.h"  // Compiler is having issues with relative path here

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
// using namespace query_engine;
using namespace atoms;
using namespace link_creation_agent;
using namespace metta;

static string parse_metta_expression(string metta_expression,
                                     unordered_map<string, string>& handle_to_metta,
                                     map<string, string>& assignment_table) {
    auto atomdb = AtomDBSingleton::get_instance();
    string metta_expression_cp = metta_expression;
    for (const auto& assignment_pair : assignment_table) {
        if (handle_to_metta.find(assignment_pair.first) == handle_to_metta.end()) {
            auto atom = atomdb->get_atom(assignment_pair.second);
            if (atom == nullptr) {
                Utils::error("Atom with handle " + assignment_pair.second +
                             " not found in AtomDB while creating missing atoms for link creation.");
                continue;
            }
            handle_to_metta[assignment_pair.second] = atom->metta_representation(*atomdb.get());
        }
        Utils::replace_all(
            metta_expression_cp, "$" + assignment_pair.first, handle_to_metta[assignment_pair.second]);
    }
    return metta_expression_cp;
}

static void create_missing_atoms_in_atomdb(shared_ptr<MettaParserActions> parser_actions) {
    auto atomdb = AtomDBSingleton::get_instance();
    // Add missing nodes to AtomDB
    for (const auto& element : parser_actions->handle_to_atom) {
        if (dynamic_pointer_cast<Node>(element.second) != nullptr) {
            try {
                atomdb->add_node(dynamic_pointer_cast<Node>(element.second).get(), false);
                LOG_DEBUG("Node added to AtomDB: " << element.second->to_string());
            } catch (const std::exception& e) {
                LOG_ERROR("Error adding node to AtomDB: " << e.what());
            }
        }
    }
    if (parser_actions->metta_expressions.size() == 0) {
        LOG_INFO("No MeTTa expressions found, skipping link metta processing.");
        return;
    }
    // Add missing links
    for (size_t i = 0; i < parser_actions->metta_expressions.size() - 1; i++) {
        auto metta_expression_cp = parser_actions->metta_expressions[i];
        auto handle = find_if(
            parser_actions->handle_to_metta_expression.begin(),
            parser_actions->handle_to_metta_expression.end(),
            [&metta_expression_cp](const auto& pair) { return pair.second == metta_expression_cp; });
        if (handle == parser_actions->handle_to_metta_expression.end()) {
            Utils::error("Could not find handle for metta expression: " + metta_expression_cp);
            continue;
        }
        auto link = parser_actions->handle_to_atom[handle->first];
        try {
            if (dynamic_pointer_cast<Link>(link) == nullptr) {
                Utils::error("Parsed atom is not a Link for metta expression: " + metta_expression_cp);
                continue;
            }
            atomdb->add_link(dynamic_pointer_cast<Link>(link).get(), false);
            LOG_DEBUG("Link added to AtomDB: " << metta_expression_cp);
        } catch (const std::exception& e) {
            LOG_ERROR("Error adding link to AtomDB: " << e.what());
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
        string metta_expression = param;
        LOG_DEBUG("Parsing MeTTa link template: " << metta_expression);
        auto parser_actions = make_shared<MettaParserActions>();
        metta_expression =
            parse_metta_expression(metta_expression, handle_to_metta, query_answer->assignment.table);
        MettaParser parser(metta_expression, parser_actions);
        parser.parse(true);
        create_missing_atoms_in_atomdb(parser_actions);
        if (parser_actions->element_stack.size() == 0) {
            Utils::error("Invalid MeTTa link template. Parser returned an empty stack.");
            continue;
        } else {
            auto link = parser_actions->element_stack.top();
            if (dynamic_pointer_cast<Link>(link) == nullptr) {
                Utils::error("Parsed atom is not a Link for metta expression: " + metta_expression);
                continue;
            }
            LOG_DEBUG("Parsed MeTTa link template: " << metta_expression
                                                     << " Link handle: " << link->handle());
            LOG_DEBUG("Parsed MeTTa link template: " << metta_expression
                                                     << " Link: " << link->to_string());
            links.push_back(dynamic_pointer_cast<Link>(link));
        }
    }
    return links;
}