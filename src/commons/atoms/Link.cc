#include "Link.h"

#include <string.h>

#include "HandleDecoder.h"
#include "Hasher.h"
#include "MettaMapping.h"

using namespace atoms;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors , basic operators and initializers

Link::Link(const string& type,
           const vector<string>& targets,
           bool is_toplevel,
           const Properties& custom_attributes)
    : Atom(type, is_toplevel, custom_attributes), targets(targets) {
    this->validate();
}

Link::Link(const string& type, const vector<string>& targets, const Properties& custom_attributes)
    : Atom(type, false, custom_attributes), targets(targets) {
    this->validate();
}

Link::Link(vector<string>& tokens) {
    untokenize(tokens);
}

Link::Link(const Link& other) : Atom(other) { this->targets = other.targets; }

Link& Link::operator=(const Link& other) {
    Atom::operator=(other);
    this->targets = other.targets;
    return *this;
}

bool Link::operator==(const Link& other) {
    return Atom::operator==(other) && (this->targets == other.targets);
}

bool Link::operator!=(const Link& other) { return !(*this == other); }

void Link::validate() const {
    if (this->type == Atom::UNDEFINED_TYPE) {
        Utils::error("Link type can't be '" + Atom::UNDEFINED_TYPE + "'");
    }
    if (this->targets.size() == 0) {
        Utils::error("Link must have at least 1 target");
    }
}

// -------------------------------------------------------------------------------------------------
// Public API

string Link::to_string() const {
    string result = "Link(type: '" + this->type + "', targets: [";
    if (!this->targets.empty()) {
        for (const auto& target : this->targets) {
            result += target + ", ";
        }
        result.erase(result.size() - 2);  // Remove the last comma and space
    }
    result += "], is_toplevel: " + string(this->is_toplevel ? "true" : "false") +
              ", custom_attributes: " + this->custom_attributes.to_string() + ")";
    return result;
}

string Link::handle() const { return Hasher::link_handle(this->type, this->targets); }

string Link::composite_type_hash(HandleDecoder& decoder) const {
    unsigned int arity = this->targets.size();
    unsigned int cursor = 0;
    char** handles = new char*[arity + 1];
    for (string handle : this->composite_type(decoder)) {
        handles[cursor++] = strdup(handle.c_str());
    }
    char* hash_cstr = ::composite_hash(handles, arity + 1);

    string hash_string(hash_cstr);
    free(hash_cstr);
    for (int i = arity; i >= 0; i--) {
        free(handles[i]);
    }
    delete[] handles;
    return hash_string;
}

vector<string> Link::composite_type(HandleDecoder& decoder) const {
    vector<string> composite_type;
    composite_type.push_back(named_type_hash());
    for (string handle : this->targets) {
        shared_ptr<Atom> atom = decoder.get_atom(handle);
        if (atom == nullptr) {
            Utils::error("Unkown atom with handle: " + handle);
            return {};
        } else {
            composite_type.push_back(atom->composite_type_hash(decoder));
        }
    }
    return composite_type;
}

string Link::metta_representation(HandleDecoder& decoder) const {
    if (this->type != MettaMapping::EXPRESSION_LINK_TYPE) {
        Utils::error("Can't compute metta expression of link whose type (" + this->type + ") is not " +
                     MettaMapping::EXPRESSION_LINK_TYPE);
    }
    string metta_string = "(";
    unsigned int size = this->targets.size();
    for (unsigned int i = 0; i < size; i++) {
        shared_ptr<Atom> atom = decoder.get_atom(this->targets[i]);
        if (atom == nullptr) {
            Utils::error("Couldn't decode handle: " + this->targets[i]);
            return "";
        }
        metta_string += atom->metta_representation(decoder);
        if (i != (size - 1)) {
            metta_string += " ";
        }
    }
    metta_string += ")";
    return metta_string;
}

unsigned int Link::arity() const { return this->targets.size(); }

bool Link::match(const string& handle, Assignment& assignment, HandleDecoder& decoder) {
    return this->handle() == handle;
}

void Link::tokenize(vector<string>& output) {
    output.insert(output.begin(), this->targets.begin(), this->targets.end());
    output.insert(output.begin(), std::to_string(this->targets.size()));
    Atom::tokenize(output);
}

void Link::untokenize(vector<string>& tokens) {
    Atom::untokenize(tokens);
    unsigned int num_targets = std::stoi(tokens[0]);
    this->targets.insert(this->targets.begin(), tokens.begin() + 1, tokens.begin() + 1 + num_targets);
    tokens.erase(tokens.begin(), tokens.begin() + 1 + num_targets);
}

