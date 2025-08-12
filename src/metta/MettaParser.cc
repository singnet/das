#include "MettaParser.h"

#include <stack>

#include "MettaTokens.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace metta;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initializers

MettaParser::MettaParser(shared_ptr<MettaLexer> lexer, shared_ptr<ParserActions> parser_actions) {
    this->lexer = lexer;
    _init(parser_actions);
}

MettaParser::MettaParser(const string& metta_string, shared_ptr<ParserActions> parser_actions) {
    this->lexer = make_shared<MettaLexer>(metta_string);
    _init(parser_actions);
}

void MettaParser::_init(shared_ptr<ParserActions> parser_actions) { this->actions = parser_actions; }

MettaParser::~MettaParser() {}

// -------------------------------------------------------------------------------------------------
// Public methods

bool MettaParser::parse(bool throw_on_parse_error) {
    stack<unique_ptr<Token>> token_stack;
    unique_ptr<Token> token;
    while ((token = this->lexer->next()) != nullptr) {
        LOG_DEBUG("New token: " + token->to_string());
        if (token->type == MettaTokens::OPEN_PARENTHESIS) {
            this->actions->expression_begin();
            token_stack.push(move(token));
        } else if (token->type == MettaTokens::CLOSE_PARENTHESIS) {
            while ((token_stack.size() > 0) &&
                   (token_stack.top()->type != MettaTokens::OPEN_PARENTHESIS)) {
                token_stack.pop();
            }
            if (token_stack.size() == 0) {
                _error(throw_on_parse_error, "Unmached ')'", token->text, token->type);
                return true;
            }
            token_stack.pop();
            this->actions->expression_end(token_stack.size() == 0);
        } else if (token->type == MettaTokens::SYMBOL) {
            this->actions->symbol(token->text);
            token_stack.push(move(token));
        } else if (token->type == MettaTokens::VARIABLE) {
            this->actions->variable(token->text);
            token_stack.push(move(token));
        } else if (token->type == MettaTokens::STRING_LITERAL) {
            this->actions->literal(token->text);
            token_stack.push(move(token));
        } else if (token->type == MettaTokens::INTEGER_LITERAL) {
            this->actions->literal(Utils::string_to_int(token->text));
            token_stack.push(move(token));
        } else if (token->type == MettaTokens::FLOAT_LITERAL) {
            this->actions->literal(Utils::string_to_float(token->text));
            token_stack.push(move(token));
        } else {
            _error(throw_on_parse_error, "Unexpected token", token->text, token->type);
        }
    }
    while (token_stack.size() > 0) {
        token = move(token_stack.top());
        token_stack.pop();
        if ((token->type == MettaTokens::OPEN_PARENTHESIS) ||
            (token->type == MettaTokens::CLOSE_PARENTHESIS)) {
            _error(throw_on_parse_error,
                   "Invalid remaining '(' or ')' tokens after matching ()'s",
                   token->text,
                   token->type);
            return true;
        }
    }
    return false;
}

// -------------------------------------------------------------------------------------------------
// Private methods

void MettaParser::_error(bool throw_flag,
                         const string& error_message,
                         const string& token_text,
                         unsigned char token_type) {
    static map<unsigned char, string> token_name;
    token_name[1] = "OPEN_PARENTHESIS";
    token_name[2] = "CLOSE_PARENTHESIS";
    token_name[64] = "SYMBOL";
    token_name[129] = "STRING_LITERAL";
    token_name[130] = "INTEGER_LITERAL";
    token_name[131] = "FLOAT_LITERAL";
    token_name[192] = "VARIABLE";

    string prefix = "MettaParser error in line " + std::to_string(this->lexer->line_number) + " near <" +
                    token_text + "> (" + token_name[token_type] + ")";
    Utils::error(prefix + " - " + error_message, throw_flag);
}
