#include <iostream> // XXX
#include "LinkSchema.h"

#include "Hasher.h"
#include "Link.h"
#include "Node.h"
#include "UntypedVariable.h"

using namespace atoms;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors , basic operators and initializers

LinkSchema::LinkSchema(const string& type, unsigned int arity, const Properties& custom_attributes)
    : Wildcard(type, custom_attributes) {
    this->_frozen = false;
    this->_arity = arity;
    this->_schema.reserve(arity);
    this->_composite_type.reserve(arity + 1);
    this->_composite_type.push_back(named_type_hash());
    this->_metta_representation = "(";
}

bool LinkSchema::check_not_frozen() {
    if (this->_frozen) {
        Utils::error("Can't change a LinkTemplate after calling build()");
        return true;
    } else {
        return false;
    }
}

LinkSchema::LinkSchema(const LinkSchema& other) : Wildcard(other) { *this = other; }

LinkSchema& LinkSchema::operator=(const LinkSchema& other) {
    if (!other._frozen) {
        Utils::error("Can't clone an unfrozen LinkTemplate");
    }
    Wildcard::operator=(other);
    this->_arity = other._arity;
    this->_frozen = other._frozen;
    this->_schema = other._schema;
    this->_composite_type = other._composite_type;
    this->_composite_type_hash = other._composite_type_hash;
    this->_atom_handle = other._atom_handle;
    this->_metta_representation = other._metta_representation;
    return *this;
}

bool LinkSchema::operator==(const LinkSchema& other) { return this->_atom_handle == other._atom_handle; }

bool LinkSchema::operator!=(const LinkSchema& other) { return !(*this == other); }

void LinkSchema::validate() const {
    if (this->type == Atom::UNDEFINED_TYPE) {
        Utils::error("LinkSchema type can't be '" + Atom::UNDEFINED_TYPE + "'");
    }
    if (this->_schema.size() == 0) {
        Utils::error("LinkSchema must have at least 1 target");
    }
    bool flag = true;
    for (string schema_handle : this->_schema) {
        if (schema_handle == Atom::WILDCARD_HANDLE) {
            flag = false;
            break;
        }
    }
    if (flag) {
        Utils::error("Invalid LinkSchema with no variables and no nested link schemas");
    }
}

// -------------------------------------------------------------------------------------------------
// Public Atom API

string LinkSchema::to_string() const {
    if (!this->_frozen) {
        Utils::error("Can't call to_string() before finishing LinkTemplate by calling build()");
    }
    string result = "LinkSchema(type: '" + this->type + "', targets: [";
    if (!this->_schema.empty()) {
        for (const auto& target : this->_schema) {
            result += target + ", ";
        }
        result.erase(result.size() - 2);  // Remove the last comma and space
    }
    result += "], custom_attributes: " + this->custom_attributes.to_string() + ")";
    return result;
}

string LinkSchema::handle() const { return this->_atom_handle; }

string LinkSchema::composite_type_hash(HandleDecoder& decoder) const {
    return this->_composite_type_hash;
}

vector<string> LinkSchema::composite_type(HandleDecoder& decoder) const { return this->_composite_type; }

string LinkSchema::metta_representation(HandleDecoder& decoder) const {
    return this->_metta_representation;
}

unsigned int LinkSchema::arity() const {
    return this->_arity;
}

// -------------------------------------------------------------------------------------------------
// Public API to build LinkSchema objects

void LinkSchema::add_target(const string& schema_handle,
                            const string& composite_type_hash,
                            const string& metta_representation) {
    if (!this->_frozen) {
        this->_schema.push_back(schema_handle);
        this->_composite_type.push_back(composite_type_hash);
        this->_metta_representation += metta_representation;
        if (this->_schema.size() == _arity) {
            this->_frozen = true;
            this->_metta_representation += ")";
            this->_atom_handle = Hasher::link_handle(type, this->_schema);
            this->_composite_type_hash = Hasher::composite_handle(this->_composite_type);
            this->validate();
        } else {
            this->_metta_representation += " ";
        }
    } else {
        Utils::error("Attempt to add a new target beyond LinkSchema's arity. LinkSchema: " +
                     this->to_string());
    }
}

void LinkSchema::stack_node(const string& type, const string& name) {
    if (!check_not_frozen()) {
        tuple<string, string, string> triplet(
            Hasher::node_handle(type, name), Hasher::type_handle(type), name);
        this->_atom_stack.push_back(triplet);
    }
}

void LinkSchema::stack_untyped_variable(const string& name) {
    if (!check_not_frozen()) {
        UntypedVariable variable(name);
        tuple<string, string, string> triplet(
            variable.schema_handle(), variable.named_type_hash(), "$" + name);
        this->_atom_stack.push_back(triplet);
    }
}

void LinkSchema::stack_link(const string& type, unsigned int link_arity) {
    if (check_not_frozen()) {
        return;
    }
    if (this->_atom_stack.size() >= link_arity) {
        vector<string> target_handles;
        vector<string> composite_type;
        string metta_expression = "";
        target_handles.reserve(link_arity);
        composite_type.reserve(link_arity);
        bool first = true;
        for (int i = link_arity - 1; i >= 0; i--) {
            tuple<string, string, string> triplet = this->_atom_stack.back();
            target_handles.insert(target_handles.begin(), get<0>(triplet));
            composite_type.insert(composite_type.begin(), get<1>(triplet));
            //target_handles.push_back(get<0>(triplet));
            //composite_type.push_back(get<1>(triplet));
            if (first) {
                metta_expression = get<2>(triplet) + ")";
                first = false;
            } else {
                metta_expression = get<2>(triplet) + " " + metta_expression;
            }
            this->_atom_stack.pop_back();
        }
        metta_expression = "(" + metta_expression;
        // TODO inval;id metta_expression with something not Expression/Symbol
        tuple<string, string, string> triplet(Hasher::link_handle(type, target_handles),
                                              Hasher::composite_handle(composite_type),
                                              metta_expression);
        this->_atom_stack.push_back(triplet);
    } else {
        Utils::error("Couldn't stack link. Link arity: " + std::to_string(link_arity) +
                     " stack size: " + std::to_string(this->_atom_stack.size()));
    }
}

void LinkSchema::build() {
    if (!check_not_frozen()) {
        if (this->_atom_stack.size() == this->_arity) {
            for (auto triplet: this->_atom_stack) {
                add_target(get<0>(triplet), get<1>(triplet), get<2>(triplet));
            }
            this->_atom_stack.clear();
        } else {
            Utils::error("Can't build LinkTemplate of arity " + std::to_string(this->_arity) +
                         " out of a stack with " + std::to_string(this->_atom_stack.size()) + " atoms.");
        }
    }
}
