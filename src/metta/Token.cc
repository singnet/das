#include "Token.h"

using namespace metta;

// -------------------------------------------------------------------------------------------------
// Public methods

Token::Token() {
    this->value = 0;
    this->text = "";
}

Token::Token(unsigned char value) {
    this->value = value;
    this->text = "";
}

Token::Token(const Token& other) {
    this->value = other.value;
    this->text = other.text;
}

Token::Token(unsigned char value, const string& text) {
    this->value = value;
    this->text = text;
}

Token::~Token() {}

Token& Token::operator=(const Token& other) {
    this->value = other.value;
    this->text = other.text;
    return *this;
}

bool Token::operator==(const Token& other) {
    return (this->value == other.value) && (this->text == other.text);
}

bool Token::operator!=(const Token& other) { return !(*this == other); }
