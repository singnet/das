#include "UntypedVariable.h"

#include "Hasher.h"

using namespace atoms;

UntypedVariable::UntypedVariable(const string& name) : Wildcard(Atom::UNDEFINED_TYPE), name(name) {
    this->validate();
}

UntypedVariable::UntypedVariable(const UntypedVariable& other) : Wildcard(other) {
    this->name = other.name;
}

UntypedVariable& UntypedVariable::operator=(const UntypedVariable& other) {
    Wildcard::operator=(other);
    this->name = other.name;
    return *this;
}

bool UntypedVariable::operator==(const UntypedVariable& other) {
    return (this->name == other.name) && Wildcard::operator==(other);
}

bool UntypedVariable::operator!=(const UntypedVariable& other) { return !(*this == other); }

void UntypedVariable::validate() const {
    if (this->type != Atom::UNDEFINED_TYPE) {
        Utils::error("Invalid type for UntypedVariable: " + this->type + " (expected " +
                     Atom::UNDEFINED_TYPE + ")");
    }
    if (this->name.empty()) {
        Utils::error("Invalid empty name for UntypedVariable");
    }
}

string UntypedVariable::to_string() const { return "UntypedVariable(name: '" + this->name + "')"; }

string UntypedVariable::handle() const { return Hasher::node_handle(this->type, this->name); }

string UntypedVariable::metta_representation(HandleDecoder& decoder) const { return "$" + this->name; }

bool UntypedVariable::match(const string& handle, Assignment& assignment, HandleDecoder& decoder) {
    return assignment.assign(this->name, handle);
}
