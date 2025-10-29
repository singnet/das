#include "MettaTemplateProcessor.h"

#include "AtomDBSingleton.h"
#include "MettaParser.h"
#include "commons/atoms/MettaParserActions.h"  // Compiler is having issues with relative path here

using namespace std;
// using namespace query_engine;
using namespace atoms;
using namespace link_creation_agent;
using namespace metta;

vector<shared_ptr<Link>> MettaTemplateProcessor::process_query(shared_ptr<QueryAnswer> query_answer,
                                                               optional<vector<string>> extra_params) {
    vector<shared_ptr<Link>> links;
    if (extra_params == nullopt) {
        Utils::error("Invalid link template");
    }
    auto atomdb = AtomDBSingleton::get_instance();
    for (const auto& param : extra_params.value()) {
        auto parser_actions = make_shared<MettaParserActions>();
        string metta_expression = param;
        for (const auto& assignment_pair : query_answer->assignment.table) {
            auto atom = atomdb->get_atom(assignment_pair.second);
            string metta_assigment;
            if (atom != nullptr) {
                metta_assigment = atom->metta_representation(*atomdb.get());
            } else {
                Utils::error("Invalid MeTTa link template, could not get MeTTa expression for handle: " +
                             assignment_pair.second);
            }
            Utils::replace_all(metta_expression, "$" + assignment_pair.first, metta_assigment);
        }
        LOG_DEBUG("Parsing MeTTa link template: " << metta_expression);
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