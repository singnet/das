#pragma once

#include <memory>
#include "MettaLexer.h"
#include "ParserActions.h"

using namespace std;

namespace metta {

/**
 * A configurable parser of MeTTa input. It's configurable in the sense that the actual
 * parser actions (i.e. the methods called as the elements of the MeTTa syntax are scanned
 * and parsed) can be extended and changed to fit different needs.
 *
 * The parser can work on a plain string or on some more complex inputs (e.g. a file,
 * a list of strings or a list of files). The simpler way to use it is just to use the constructor
 * whi9ch expects a MeTTa string. To parse file(s) or more than one string at once, one should
 * use the constructor that expects a MettaLexer instead.
 */
class MettaParser {

public:

    /**
     * Basic constructor which expects a single MeTTa string to be parsed.
     *
     * @param metta_string Parser input.
     * @param parser_actions Object that is called back as elements are parsed from the input.
     */
    MettaParser(const string& metta_string, shared_ptr<ParserActions> parser_actions = nullptr);

    /**
     * Constructor which can be used to parse more complex entries such as files, list of strings
     * or list of files.
     *
     * @param lexer MettaLexer object initialized according to the input to be used.
     * @param parser_actions Object that is called back as elements are parsed from the input.
     */
    MettaParser(shared_ptr<MettaLexer> lexer, shared_ptr<ParserActions> parser_actions = nullptr);

    /**
     * Destructor.
     */
    ~MettaParser();

    /**
     * Method used to actually parse the input. A successful parse will return 'false'. If any
     * syntax or lexer error occur, the method returns 'true'.
     *
     * @param throw_on_parse_error When true, syntax or lexer errors will throw an exception. If
     * this flag is false, errors will still be logged using LOG_ERROR but no exceptions will be thrown.
     * The success of the parse can only be determined by the method's return value.
     * @return False if parsing completed whitout any errors. True otherwise.
     */
    bool parse(bool throw_on_parse_error = true);

private:

    void _init(shared_ptr<ParserActions> parser_actions);
    void _error(bool throw_flag, const string& error_message, const string& token_text, unsigned char token_type);

    shared_ptr<MettaLexer> lexer;
    shared_ptr<ParserActions> actions;
};

} // namespace metta
