#pragma once

#include <string>

using namespace std;

namespace metta {

/**
 *
 */
class Token {
   public:
    Token();
    Token(unsigned char value);
    Token(unsigned char value, const string& text);
    Token(const Token& other);
    ~Token();

    Token& operator=(const Token& other);
    bool operator==(const Token& other);
    bool operator!=(const Token& other);

    string to_string();
    unsigned char type;
    string text;

   private:
};

}  // namespace metta
