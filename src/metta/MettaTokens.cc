#include "MettaTokens.h"

#include "Utils.h"

using namespace metta;
using namespace commons;

// Tokens can be used just as plain unsigned integers (actually unsigned char) with a different
// value assigned to each token. We use a bit-schema just to select those numbers in a way which
// can be used to group tokens that are syntaticly similar.
//
// Tokens have 8 bits. The first 2 identify which type of token it is (i.e. syntax marker,
// symbol, literal or variable). The remaining bits are used to distinguish among "sub types"
// like "integer literal" etc.

#define TOKEN_TYPE_MASK ((unsigned char) (3 << 6))

#define SYNTAX_MARKER_ID ((unsigned char) 0)
#define SYMBOL_ID ((unsigned char) 1)
#define LITERAL_ID ((unsigned char) 2)
#define VARIABLE_ID ((unsigned char) 3)

const unsigned char MettaTokens::OPEN_PARENTHESIS = (SYNTAX_MARKER_ID << 6) | 1;   // 1
const unsigned char MettaTokens::CLOSE_PARENTHESIS = (SYNTAX_MARKER_ID << 6) | 2;  // 2
const unsigned char MettaTokens::SYMBOL = (SYMBOL_ID << 6);                        // 64
const unsigned char MettaTokens::STRING_LITERAL = (LITERAL_ID << 6) | 1;           // 129
const unsigned char MettaTokens::INTEGER_LITERAL = (LITERAL_ID << 6) | 2;          // 130
const unsigned char MettaTokens::FLOAT_LITERAL = (LITERAL_ID << 6) | 3;            // 131
const unsigned char MettaTokens::VARIABLE = (VARIABLE_ID << 6);                    // 192

bool MettaTokens::is_syntax_marker(unsigned char token) {
    return ((token & TOKEN_TYPE_MASK) >> 6) == SYNTAX_MARKER_ID;
}

bool MettaTokens::is_symbol(unsigned char token) {
    return ((token & TOKEN_TYPE_MASK) >> 6) == SYMBOL_ID;
}

bool MettaTokens::is_literal(unsigned char token) {
    return ((token & TOKEN_TYPE_MASK) >> 6) == LITERAL_ID;
}

bool MettaTokens::is_variable(unsigned char token) {
    return ((token & TOKEN_TYPE_MASK) >> 6) == VARIABLE_ID;
}

string MettaTokens::to_string(unsigned char token) {
    if (token == OPEN_PARENTHESIS) {
        return "OPEN_PARENTHESIS";
    } else if (token == CLOSE_PARENTHESIS) {
        return "CLOSE_PARENTHESIS";
    } else if (token == SYMBOL) {
        return "SYMBOL";
    } else if (token == STRING_LITERAL) {
        return "STRING_LITERAL";
    } else if (token == INTEGER_LITERAL) {
        return "INTEGER_LITERAL";
    } else if (token == FLOAT_LITERAL) {
        return "FLOAT_LITERAL";
    } else if (token == VARIABLE) {
        return "VARIABLE";
    } else {
        Utils::error("Unknown token: " + std::to_string(token));
        return "";
    }
}
