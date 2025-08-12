#pragma once

#include <memory>
#include <stack>

#include "Token.h"

using namespace std;

namespace metta {

/**
 * This class holds the actions which are supposed to be taken during the parsing
 * os a MeTTa input. Default behavior is just to log which methods are being called.
 *
 * This class is supposed to be extended in order to provide real actions for the parser
 * but these concrete implementations depends on the way the parser is being used
 * (e.g. building LinkSchema or reading a file to populate AtomDB). Such concrete
 * subclasses are supposed to be provided in the package using the parser.
 */
class ParserActions {
   public:
    ParserActions();
    virtual ~ParserActions();

    /**
     * Action called when a symbol is parsed.
     */
    virtual void symbol(const string& name);

    /**
     * Action called when a variable is parsed.
     */
    virtual void variable(const string& value);

    /**
     * Action called when a string literal is parsed.
     */
    virtual void literal(const string& value);

    /**
     * Action called when a integer literal is parsed.
     */
    virtual void literal(int value);

    /**
     * Action called when a float literal is parsed.
     */
    virtual void literal(float value);

    /**
     * Action called when the beggining of an expression is parsed (i.e. after the
     * corresponding '(' token).
     */
    virtual void expression_begin();

    /**
     * Action called when an expression is fully parsed (i.e. after the corresponding ')' token).
     *
     * @param toplevel A flag to indicate whether the expression is a toplevel onr or not.
     */
    virtual void expression_end(bool toplevel);
};

}  // namespace metta
