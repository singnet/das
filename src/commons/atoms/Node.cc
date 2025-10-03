#include "Node.h"

#include "Hasher.h"
#include "MettaMapping.h"

using namespace atoms;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors , basic operators and initializers

Node::Node(const string& type, const string& name, bool is_toplevel, const Properties& custom_attributes)
    : Atom(type, is_toplevel, custom_attributes), name(name) {
    this->validate();
}

Node::Node(const string& type, const string& name, const Properties& custom_attributes)
    : Atom(type, false, custom_attributes), name(name) {
    this->validate();
}

Node::Node(const Node& other) : Atom(other) { this->name = other.name; }

Node& Node::operator=(const Node& other) {
    Atom::operator=(other);
    this->name = other.name;
    return *this;
}

bool Node::operator==(const Node& other) {
    return (this->name == other.name) && Atom::operator==(other);
}

bool Node::operator!=(const Node& other) { return !(*this == other); }

void Node::validate() const {
    if (this->type == Atom::UNDEFINED_TYPE) {
        Utils::error("Node type can't be '" + Atom::UNDEFINED_TYPE + "'");
    }
    if (this->name.empty()) {
        Utils::error("Node name must not be empty");
    }
}

// -------------------------------------------------------------------------------------------------
// Public API

string Node::to_string() const {
    return "Node(type: '" + this->type + "', name: '" + this->name +
           "', custom_attributes: " + this->custom_attributes.to_string() + ")";
}

string Node::handle() const { return Hasher::node_handle(this->type, this->name); }

string Node::metta_representation(HandleDecoder& decoder) const {
    if (this->type != MettaMapping::SYMBOL_NODE_TYPE) {
        Utils::error("Can't compute metta expression of node whose type (" + this->type + ") is not " +
                     MettaMapping::SYMBOL_NODE_TYPE);
    }
    return this->name;
}

bool Node::match(const string& handle, Assignment& assignment, HandleDecoder& decoder) {
    return this->handle() == handle;
}

void Node::tokenize(vector<string>& output) {
    output.insert(output.begin(), this->name);
    Atom::tokenize(output);
}

void Node::untokenize(vector<string>& tokens) {
    Atom::untokenize(tokens);
    this->name = tokens[0];
    tokens.erase(tokens.begin(), tokens.begin() + 1);
}

