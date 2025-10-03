#include "Wildcard.h"

using namespace atoms;

Wildcard::Wildcard(const string& type, bool is_toplevel, const Properties& custom_attributes)
    : Atom(type, is_toplevel, custom_attributes) {
    this->validate();
}

Wildcard::Wildcard(const string& type, const Properties& custom_attributes)
    : Atom(type, false, custom_attributes) {
    this->validate();
}

Wildcard::Wildcard(const Wildcard& other) : Atom(other) {}

Wildcard& Wildcard::operator=(const Wildcard& other) {
    Atom::operator=(other);
    return *this;
}

bool Wildcard::operator==(const Wildcard& other) { return Atom::operator==(other); }

bool Wildcard::operator!=(const Wildcard& other) { return !(*this == other); }

void Wildcard::validate() const {
    Atom::validate();  // Validate type
}

string Wildcard::to_string() const { return "Wildcard(type: '" + this->type + "')"; }

string Wildcard::schema_handle() const { return Atom::WILDCARD_STRING; }

void Wildcard::tokenize(vector<string>& output) {
    Atom::tokenize(output);
}

void Wildcard::untokenize(vector<string>& tokens) {
    Atom::untokenize(tokens);
}
