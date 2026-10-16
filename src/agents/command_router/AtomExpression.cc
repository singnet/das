#include "AtomExpression.h"

#include <memory>
#include <set>
#include <string>

#include "Link.h"
#include "MettaParser.h"
#include "MettaParserActions.h"
#include "Node.h"

using namespace std;
using namespace atoms;
using namespace atomdb;
using namespace metta;

namespace {

shared_ptr<MettaParserActions> parse_metta_atoms(const string& expression) {
    auto actions = make_shared<MettaParserActions>();
    MettaParser parser(expression, actions);
    if (parser.parse(false) || actions->handle_to_atom.empty()) {
        RAISE_ERROR("Invalid MeTTa expression: " + expression);
    }
    return actions;
}

string root_handle(const MettaParserActions& actions) {
    if (!actions.metta_expression_handle.empty()) {
        return actions.metta_expression_handle;
    }
    if (actions.handle_to_atom.size() == 1) {
        return actions.handle_to_atom.begin()->first;
    }
    RAISE_ERROR("MeTTa expression did not produce an atom");
    return "";
}

void add_parsed_atom(const shared_ptr<Atom>& atom,
                     MettaParserActions& actions,
                     AtomDB& db,
                     set<string>& added) {
    const string handle = atom->handle();
    if (!added.insert(handle).second) {
        return;
    }
    if (auto link = dynamic_pointer_cast<Link>(atom)) {
        for (const string& target : link->targets) {
            auto target_it = actions.handle_to_atom.find(target);
            if (target_it != actions.handle_to_atom.end()) {
                add_parsed_atom(target_it->second, actions, db, added);
            }
        }
        auto metta_it = actions.handle_to_metta_expression.find(handle);
        if (metta_it != actions.handle_to_metta_expression.end()) {
            link->custom_attributes["metta_expression"] = metta_it->second;
        }
        db.add_link(link.get());
    } else if (auto node = dynamic_pointer_cast<Node>(atom)) {
        db.add_node(node.get());
    } else {
        db.add_atom(atom.get());
    }
}

}  // namespace

string command_router::add_metta_expression(AtomDB& db, const string& expression) {
    auto actions = parse_metta_atoms(expression);
    const string handle = root_handle(*actions);
    set<string> added;
    add_parsed_atom(actions->handle_to_atom.at(handle), *actions, db, added);
    return handle;
}

string command_router::metta_expression_root_handle(const string& expression) {
    return root_handle(*parse_metta_atoms(expression));
}
