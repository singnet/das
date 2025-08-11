#include "Token.h"
#include "MettaTokens.h"

using namespace metta;

// -------------------------------------------------------------------------------------------------
// Public methods

Token::Token() {
    this->type = 0;
    this->text = "";
}

Token::Token(unsigned char type) {
    this->type = type;
    this->text = "";
}

Token::Token(const Token& other) {
    this->type = other.type;
    this->text = other.text;
}

Token::Token(unsigned char type, const string& text) {
    this->type = type;
    this->text = text;
}

Token::~Token() {}

Token& Token::operator=(const Token& other) {
    this->type = other.type;
    this->text = other.text;
    return *this;
}

bool Token::operator==(const Token& other) {
    return (this->type == other.type) && (this->text == other.text);
}

bool Token::operator!=(const Token& other) { return !(*this == other); }

string Token::to_string() {
    return "<" + MettaTokens::to_string(this->type) + ", " + this->text + ">";
}
