#include <iostream>
#include <memory>
#include <string>

#include "HandleDecoder.h"
#include "LinkSchema.h"
#include "MettaMapping.h"

using namespace std;
using namespace atoms;
using namespace commons;

// metta_representation() requires a decoder, but LinkSchema already stores the text.
class UnusedDecoder : public HandleDecoder {
   public:
    shared_ptr<Atom> get_atom(const string& handle) override { return nullptr; }
};

// (Evaluation (Predicate $r) (List (Concept "<team>") (Concept $v)))
// Nested link schemas are hashed as Atom::WILDCARD_STRING, so the concrete
// Concept name does not change the top-level handle.
static LinkSchema evaluation_schema(const string& team_name) {
    string symbol = MettaMapping::SYMBOL_NODE_TYPE;
    string expression = MettaMapping::EXPRESSION_LINK_TYPE;

    LinkSchema schema(expression, 3);
    schema.stack_node(symbol, "Evaluation");

    schema.stack_node(symbol, "Predicate");
    schema.stack_untyped_variable("r");
    schema.stack_link_schema(expression, 2);

    schema.stack_node(symbol, "List");
    schema.stack_node(symbol, "Concept");
    schema.stack_node(symbol, "\"" + team_name + "\"");
    schema.stack_link_schema(expression, 2);
    schema.stack_node(symbol, "Concept");
    schema.stack_untyped_variable("v");
    schema.stack_link_schema(expression, 2);
    schema.stack_link_schema(expression, 3);

    schema.build();
    return schema;
}

static void print_schema(const string& label, LinkSchema& schema, HandleDecoder& decoder) {
    cout << label << endl;
    cout << "  metta:   " << schema.metta_representation(decoder) << endl;
    cout << "  handle:  " << schema.handle() << endl;
    cout << "  targets:";
    for (const string& target : schema.targets()) {
        cout << " " << target;
    }
    cout << endl;
}

int main() {
    UnusedDecoder decoder;
    LinkSchema team_a = evaluation_schema("team a");
    LinkSchema team_b = evaluation_schema("team b");

    print_schema("schema A", team_a, decoder);
    print_schema("schema B", team_b, decoder);

    bool same_handle = team_a.handle() == team_b.handle();
    cout << "same handle: " << (same_handle ? "yes" : "no") << endl;
    return same_handle ? 0 : 1;
}
