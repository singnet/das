#pragma once

#include <stack>

#include "Atom.h"
#include "ParserActions.h"

using namespace std;
using namespace metta;
using namespace atoms;

namespace atoms {

enum ExpressionType { LINK, AND, OR };

/**
 *
 */
class MettaParserActions : public ParserActions {
   public:
    MettaParserActions();
    ~MettaParserActions();

    /**
     * Action called when a symbol is parsed.
     */
    void symbol(const string& name) override;

    /**
     * Action called when a variable is parsed.
     */
    void variable(const string& value) override;

    /**
     * Action called when a string literal is parsed.
     */
    void literal(const string& value) override;

    /**
     * Action called when a integer literal is parsed.
     */
    void literal(int value) override;

    /**
     * Action called when a float literal is parsed.
     */
    void literal(float value) override;

    /**
     * Action called when the beggining of an expression is parsed (i.e. after the
     * corresponding '(' token).
     */
    void expression_begin() override;

    /**
     * Action called when an expression is fully parsed (i.e. after the corresponding ')' token).
     *
     * @param toplevel A flag to indicate whether the expression is a toplevel onr or not.
     */
    void expression_end(bool toplevel, const string& metta_string) override;

    stack<shared_ptr<Atom>> element_stack;

    // The handle of the metta expression that is being parsed (i.e. the top-level expression).
    string metta_expression_handle;

    // A map with all parsed atoms, indexed by their handle.
    map<string, shared_ptr<Atom>> handle_to_atom;

    // A map with all parsed metta expressions, indexed by their handle.
    map<string, string> handle_to_metta_expression;

   private:
    unsigned int current_expression_size;
    ExpressionType current_expression_type;
    stack<unsigned int> expression_size_stack;
    stack<ExpressionType> expression_type_stack;
};

}  // namespace atoms
