#pragma once

#include <stack>

#include "ParserActions.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryElement.h"

using namespace std;
using namespace metta;
using namespace query_element;

namespace query_engine {

enum ExpressionType { LINK, LINK_TEMPLATE, AND, OR };

/**
 *
 */
class MettaParserActions : public ParserActions {
   public:
    MettaParserActions(shared_ptr<PatternMatchingQueryProxy> proxy);
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
    void expression_begin();

    /**
     * Action called when an expression is fully parsed (i.e. after the corresponding ')' token).
     *
     * @param toplevel A flag to indicate whether the expression is a toplevel onr or not.
     */
    void expression_end(bool toplevel);

    stack<shared_ptr<QueryElement>> element_stack;

   private:
    shared_ptr<PatternMatchingQueryProxy> proxy;
    unsigned int current_expression_size;
    ExpressionType current_expression_type;
    stack<unsigned int> expression_size_stack;
    stack<ExpressionType> expression_type_stack;
};

}  // namespace query_engine
