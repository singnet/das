#include "Metta2AtomsMapper.h"

#include <variant>

#include "MettaParser.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace db_adapter;
using namespace commons;
using namespace atoms;
using namespace metta;

// ==============================
//  Construction / destruction
// ==============================

Metta2AtomsMapper::Metta2AtomsMapper() { this->parser_actions = make_shared<MettaParserActions>(); }

Metta2AtomsMapper::~Metta2AtomsMapper() {}

// ==============================
//  Public
// ==============================

const vector<Atom*> Metta2AtomsMapper::map(const DbInput& data) {
    MettaExpression metta_expression = get<MettaExpression>(data);

    MettaParser parser(metta_expression.expression, this->parser_actions);
    parser.parse();

    stack<shared_ptr<Atom>> element_stack = this->parser_actions->element_stack;

    vector<Atom*> atoms;

    while (!element_stack.empty()) {
        shared_ptr<Atom> atom = element_stack.top();
        element_stack.pop();
        atoms.push_back(atom.get());
    }

    reverse(atoms.begin(), atoms.end());

    return atoms;
}