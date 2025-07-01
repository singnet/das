#include "Atom.h"
#include "Hasher.h"

using namespace commons;
using namespace atoms;

string Atom::UNDEFINED_TYPE = "__UNDEFINED_TYPE__";
string Atom::WILDCARD_STRING = "*";

// The minor memory leak below is disregarded for the sake of code simplicity
string Atom::WILDCARD_HANDLE = string(::compute_hash((char*) Atom::WILDCARD_STRING.c_str()));

// -------------------------------------------------------------------------------------------------
// Constructors, destructors , basic operators and initializers


Atom::Atom(const string& type, const Properties& custom_attributes)
    : type(type), custom_attributes(custom_attributes) {
    this->validate();
}

Atom::Atom(const Atom& other) {
    this->type = other.type;
    this->custom_attributes = other.custom_attributes;
}

Atom& Atom::operator=(const Atom& other) {
    this->type = other.type;
    this->custom_attributes = other.custom_attributes;
    return *this;
}

bool Atom::operator==(const Atom& other) {
    return (this->type == other.type) && (this->custom_attributes == other.custom_attributes);
}

bool Atom::operator!=(const Atom& other) { return !(*this == other); }

void Atom::validate() const {
    if (this->type.empty()) {
        Utils::error("Atom type must not be empty");
    }
}

// -------------------------------------------------------------------------------------------------
// Default implementation for virtual public API

string Atom::to_string() const {
    return "Atom(type: '" + this->type +
           "', custom_attributes: " + this->custom_attributes.to_string() + ")";
}

string Atom::named_type_hash() const {
    return Hasher::type_handle(this->type);
}

vector<string> Atom::composite_type(HandleDecoder& decoder) const {
    return vector<string>({named_type_hash()});
}

string Atom::composite_type_hash(HandleDecoder& decoder) const { return named_type_hash(); }

string Atom::schema_handle() const {
    return this->handle();
}
