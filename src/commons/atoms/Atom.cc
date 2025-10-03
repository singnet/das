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

Atom::Atom(const string& type, bool is_toplevel, const Properties& custom_attributes)
    : type(type), is_toplevel(is_toplevel), custom_attributes(custom_attributes) {
    this->validate();
}

Atom::Atom(const Atom& other) {
    this->type = other.type;
    this->is_toplevel = other.is_toplevel;
    this->custom_attributes = other.custom_attributes;
}

Atom& Atom::operator=(const Atom& other) {
    this->type = other.type;
    this->is_toplevel = other.is_toplevel;
    this->custom_attributes = other.custom_attributes;
    return *this;
}

bool Atom::operator==(const Atom& other) {
    return (this->type == other.type) && (this->is_toplevel == other.is_toplevel) &&
           (this->custom_attributes == other.custom_attributes);
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
           "', is_toplevel: " + string(this->is_toplevel ? "true" : "false") +
           ", custom_attributes: " + this->custom_attributes.to_string() + ")";
}

string Atom::named_type_hash() const { return Hasher::type_handle(this->type); }

vector<string> Atom::composite_type(HandleDecoder& decoder) const {
    return vector<string>({named_type_hash()});
}

string Atom::composite_type_hash(HandleDecoder& decoder) const { return named_type_hash(); }

string Atom::schema_handle() const { return this->handle(); }

unsigned int Atom::arity() const { return 0; }

void Atom::tokenize(vector<string>& output) {
    vector<string> parameters_tokens = this->custom_attributes.tokenize();
    parameters_tokens.insert(parameters_tokens.begin(), std::to_string(parameters_tokens.size()));
    output.insert(output.begin(), parameters_tokens.begin(), parameters_tokens.end());
    output.insert(output.begin(), (this->is_toplevel ? "true": "false"));
    output.insert(output.begin(), this->type);
}

void Atom::untokenize(vector<string>& tokens) {
    this->type = tokens[0];
    this->is_toplevel = (tokens[1] == "true");
    tokens.erase(tokens.begin(), tokens.begin() + 2);
    unsigned int num_property_tokens =
        Utils::string_to_int(tokens[0]);  // safe conversion, should always be a number
    if (num_property_tokens > 0) {
        vector<string> properties_tokens;
        properties_tokens.insert(
            properties_tokens.begin(), tokens.begin() + 1, tokens.begin() + 1 + num_property_tokens);
        this->custom_attributes.untokenize(properties_tokens);
        tokens.erase(tokens.begin(), tokens.begin() + 1 + num_property_tokens);
    } else {
        // If no properties are provided, we still need to remove the first token
        tokens.erase(tokens.begin());
    }
}
