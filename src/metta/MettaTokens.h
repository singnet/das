#pragma once

using namespace std;

namespace metta {

/**
 *
 */
class MettaTokens {
   public:
    static const unsigned char OPEN_PARENTHESIS;
    static const unsigned char CLOSE_PARENTHESIS;
    static const unsigned char SYMBOL;
    static const unsigned char STRING_LITERAL;
    static const unsigned char INTEGER_LITERAL;
    static const unsigned char FLOAT_LITERAL;
    static const unsigned char VARIABLE;

    static bool is_syntax_marker(unsigned char);
    static bool is_symbol(unsigned char);
    static bool is_literal(unsigned char);
    static bool is_variable(unsigned char);

    ~MettaTokens();

   private:
    MettaTokens() {}
};

}  // namespace metta
