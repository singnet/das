#include "Metta2AtomsMapper.h"

#include "Link.h"
#include "MettaParser.h"
#include "MettaParserActions.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace db_adapter;
using namespace commons;
using namespace atoms;

// ---------------------------------------------------------------------------------------------
//  Construction / destruction

Metta2AtomsMapper::Metta2AtomsMapper() {}

Metta2AtomsMapper::~Metta2AtomsMapper() {}

// ---------------------------------------------------------------------------------------------
//  Public

void Metta2AtomsMapper::map(const DbInput& data, std::queue<shared_ptr<Atom>>& output) {
    MettaExpression metta_expression = get<MettaExpression>(data);

    LOG_DEBUG("[Metta2AtomsMapper::map] Parsing MORK expression: " << metta_expression.expression);

    auto parser_actions = make_shared<MettaParserActions>();
    MettaParser parser(metta_expression.expression, parser_actions);
    parser.parse();

    stack<shared_ptr<Atom>> element_stack = parser_actions->element_stack;
    shared_ptr<Atom> atom = element_stack.top();
    auto link_toplevel = dynamic_pointer_cast<Link>(atom);

    this->collect_atoms(output, link_toplevel->handle(), parser_actions);

#if LOG_LEVEL >= DEBUG_LEVEL
    LOG_DEBUG("The expression " << metta_expression.expression
                                << " has been mapped to the following atoms:");

    for (const auto& atom : this->atoms) {
        LOG_DEBUG("-> " << atom->to_string());
    }
#endif
}

void Metta2AtomsMapper::collect_atoms(std::queue<shared_ptr<Atom>>& output,
                                      const string& handle,
                                      shared_ptr<MettaParserActions> parser_actions) {
    shared_ptr<Atom> atom = parser_actions->handle_to_atom[handle];
    if (atom != nullptr) {
        if (Atom::is_node(*atom)) {
            output.push(atom);
        } else {
            this->collect_atoms_recursive(output, dynamic_pointer_cast<Link>(atom), parser_actions);
        }
    }
}

// ---------------------------------------------------------------------------------------------
//  Private

void Metta2AtomsMapper::collect_atoms_recursive(std::queue<shared_ptr<Atom>>& output,
                                                shared_ptr<Link> link,
                                                shared_ptr<MettaParserActions> parser_actions) {
    for (string& target_handle : link->targets) {
        shared_ptr<Atom> atom = parser_actions->handle_to_atom[target_handle];
        if (atom != nullptr) {
            if (Atom::is_node(*atom)) {
                output.push(atom);
            } else {
                this->collect_atoms_recursive(output, dynamic_pointer_cast<Link>(atom), parser_actions);
            }
        }
    }
    output.push(link);
}