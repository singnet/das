#include "LinkSchema.h"

#include "HandleDecoder.h"
#include "Hasher.h"
#include "Link.h"
#include "Node.h"
#include "UntypedVariable.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atoms;

string LinkSchema::ATOM = "ATOM";
string LinkSchema::NODE = "NODE";
string LinkSchema::LINK = "LINK";
string LinkSchema::UNTYPED_VARIABLE = "VARIABLE";
string LinkSchema::LINK_TEMPLATE = "LINK_TEMPLATE";

// -------------------------------------------------------------------------------------------------
// Constructors, destructors , basic operators and initializers

LinkSchema::LinkSchema(const string& type, unsigned int arity, const Properties& custom_attributes)
    : Wildcard(type, custom_attributes) {
    _init(arity);
}

LinkSchema::LinkSchema(const vector<string>& tokens, const Properties& custom_attributes)
    : Wildcard(tokens[1], custom_attributes) {
    _syntax_assert(tokens.size() > 3, tokens[0], 0);
    _init((unsigned int) Utils::string_to_int(tokens[2]));
    untokenize(tokens);
}

void LinkSchema::_init(unsigned int arity) {
    this->_frozen = false;
    this->_arity = arity;
    this->_schema.reserve(arity);
    this->_composite_type.reserve(arity + 1);
    this->_composite_type.push_back(named_type_hash());
    this->_metta_representation = "(";
    this->_schema_element.is_link = true;

    this->_build_tokens.push_back({LINK_TEMPLATE, this->type, std::to_string(arity)});
}

bool LinkSchema::_check_not_frozen() const {
    if (this->_frozen) {
        Utils::error("Can't change a LinkTemplate after calling build()");
        return true;
    } else {
        return false;
    }
}

bool LinkSchema::_check_frozen() const {
    if (!this->_frozen) {
        Utils::error("Can't access LinkTemplate state before calling build()");
        return true;
    } else {
        return false;
    }
}

LinkSchema::LinkSchema(const LinkSchema& other) : Wildcard(other) { *this = other; }

LinkSchema& LinkSchema::operator=(const LinkSchema& other) {
    if (!other._frozen) {
        Utils::error("Can't clone an under-construction LinkTemplate");
    }
    Wildcard::operator=(other);
    this->_arity = other._arity;
    this->_frozen = other._frozen;
    this->_schema = other._schema;
    this->_composite_type = other._composite_type;
    this->_composite_type_hash = other._composite_type_hash;
    this->_atom_handle = other._atom_handle;
    this->_metta_representation = other._metta_representation;
    this->_tokens = other._tokens;
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
        if (schema_handle == Atom::WILDCARD_STRING) {
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
    string result = "";
    if (!_check_frozen()) {
        result = "LinkSchema(type: '" + this->type + "', targets: [";
        if (!this->_schema.empty()) {
            for (const auto& target : this->_schema) {
                result += target + ", ";
            }
            result.erase(result.size() - 2);  // Remove the last comma and space
        }
        result += "], custom_attributes: " + this->custom_attributes.to_string() + ")";
    }
    return result;
}

string LinkSchema::handle() const { return this->_atom_handle; }

const vector<string>& LinkSchema::targets() const { return this->_schema; }

string LinkSchema::composite_type_hash(HandleDecoder& decoder) const {
    return this->_composite_type_hash;
}

vector<string> LinkSchema::composite_type(HandleDecoder& decoder) const { return this->_composite_type; }

string LinkSchema::metta_representation(HandleDecoder& decoder) const {
    return this->_metta_representation;
}

unsigned int LinkSchema::arity() const { return this->_arity; }

bool LinkSchema::SchemaElement::match(const string& handle,
                                      Assignment& assignment,
                                      HandleDecoder& decoder,
                                      Atom* atom_ptr) {
    shared_ptr<Atom> atom;
    if (this->is_link) {
        if (atom_ptr == NULL) {
            atom = decoder.get_atom(handle);
            atom_ptr = (Link*) atom.get();
        }
        if (Atom::is_link(*atom_ptr) && (atom_ptr->arity() == this->targets.size())) {
            unsigned int cursor = 0;
            for (string target_handle : ((Link*) atom_ptr)->targets) {
                if (!this->targets[cursor++].match(target_handle, assignment, decoder, NULL)) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    } else if (this->is_wildcard) {
        // TODO: remove memory leak
        return assignment.assign(this->name, handle);
    } else {
        // is node
        return (this->handle == handle);
    }
}

bool LinkSchema::match(Link& link, Assignment& assignment, HandleDecoder& decoder) {
    return this->_schema_element.match("", assignment, decoder, &link);
}

bool LinkSchema::match(const string& handle, Assignment& assignment, HandleDecoder& decoder) {
    return this->_schema_element.match(handle, assignment, decoder, NULL);
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

void LinkSchema::stack_atom(const string& handle) {
    LOG_DEBUG("            STACK ATOM: " + handle);
    if (!_check_not_frozen()) {
        tuple<string, string, string> triplet(handle, handle, handle);
        this->_atom_stack.push_back(triplet);
        this->_build_tokens.push_back({ATOM, handle});
        SchemaElement atom_element;
        atom_element.handle = handle;
        this->_schema_element_stack.push(atom_element);
    }
}

void LinkSchema::stack_node(const string& type, const string& name) {
    LOG_DEBUG("            STACK NODE: " + type + " " + name);
    if (!_check_not_frozen()) {
        string node_handle = Hasher::node_handle(type, name);
        tuple<string, string, string> triplet(node_handle, Hasher::type_handle(type), name);
        this->_atom_stack.push_back(triplet);
        this->_build_tokens.push_back({NODE, type, name});
        SchemaElement node_element;
        node_element.name = name;
        node_element.type = type;
        node_element.handle = node_handle;
        this->_schema_element_stack.push(node_element);
    }
}

void LinkSchema::stack_untyped_variable(const string& name) {
    LOG_DEBUG("STACK UNTYPED_VARIABLE: " + name);
    if (!_check_not_frozen()) {
        UntypedVariable variable(name);
        string variable_handle = variable.schema_handle();
        tuple<string, string, string> triplet(variable_handle, variable.named_type_hash(), "$" + name);
        this->_atom_stack.push_back(triplet);
        this->_build_tokens.push_back({UNTYPED_VARIABLE, name});
        SchemaElement variable_element;
        variable_element.is_wildcard = true;
        variable_element.name = name;
        this->_schema_element_stack.push(variable_element);
    }
}

void LinkSchema::_stack_link_schema(const string& type, unsigned int link_arity, bool is_link) {
    if (_check_not_frozen()) {
        return;
    }
    if (this->_atom_stack.size() >= link_arity) {
        vector<string> target_handles;
        vector<string> composite_type;
        string metta_expression = "";
        target_handles.reserve(link_arity);
        composite_type.reserve(link_arity);
        bool first = true;
        SchemaElement new_schema_element;
        for (int i = link_arity - 1; i >= 0; i--) {
            tuple<string, string, string> triplet = this->_atom_stack.back();
            SchemaElement popped_schema_element = this->_schema_element_stack.top();
            if (is_link && (get<0>(triplet) == Atom::WILDCARD_STRING)) {
                Utils::error("Invalid wildcard in Link");
            }
            target_handles.insert(target_handles.begin(), get<0>(triplet));
            composite_type.insert(composite_type.begin(), get<1>(triplet));
            new_schema_element.targets.insert(new_schema_element.targets.begin(), popped_schema_element);
            if (first) {
                metta_expression = get<2>(triplet) + ")";
                first = false;
            } else {
                metta_expression = get<2>(triplet) + " " + metta_expression;
            }
            this->_atom_stack.pop_back();
            this->_schema_element_stack.pop();
        }
        metta_expression = "(" + metta_expression;
        // TODO check for invalid MeTTa expressions (e.g. invalid node/link types)
        string schema_handle =
            (is_link ? Hasher::link_handle(type, target_handles) : Atom::WILDCARD_STRING);
        tuple<string, string, string> triplet(
            schema_handle, Hasher::composite_handle(composite_type), metta_expression);
        this->_atom_stack.push_back(triplet);
        vector<string> new_element = {
            (is_link ? LINK : LINK_TEMPLATE), type, std::to_string(link_arity)};
        unsigned int back_cursor = this->_build_tokens.size() - link_arity;
        for (unsigned int i = 0; i < link_arity; i++) {
            new_element.insert(new_element.end(),
                               this->_build_tokens[back_cursor].begin(),
                               this->_build_tokens[back_cursor].end());
            back_cursor++;
        }
        this->_build_tokens.erase(this->_build_tokens.end() - link_arity, this->_build_tokens.end());
        this->_build_tokens.push_back(new_element);
        new_schema_element.is_link = true;
        new_schema_element.type = type;
        this->_schema_element_stack.push(new_schema_element);
    } else {
        Utils::error("Couldn't stack link. Link arity: " + std::to_string(link_arity) +
                     " stack size: " + std::to_string(this->_atom_stack.size()));
    }
}

void LinkSchema::stack_link(const string& type, unsigned int link_arity) {
    LOG_DEBUG("            STACK LINK: " + type + " " + std::to_string(link_arity));
    _stack_link_schema(type, link_arity, true);
}

void LinkSchema::stack_link_schema(const string& type, unsigned int link_arity) {
    LOG_DEBUG("     STACK LINK_SCHEMA: " + type + " " + std::to_string(link_arity));
    _stack_link_schema(type, link_arity, false);
}

void LinkSchema::build() {
    if (!_check_not_frozen()) {
        if (this->_atom_stack.size() == this->_arity) {
            for (auto triplet : this->_atom_stack) {
                add_target(get<0>(triplet), get<1>(triplet), get<2>(triplet));
            }
            this->_atom_stack.clear();
        } else {
            Utils::error("Can't build LinkTemplate of arity " + std::to_string(this->_arity) +
                         " out of a stack with " + std::to_string(this->_atom_stack.size()) + " atoms.");
        }
        for (unsigned int i = 0; i < this->_build_tokens.size(); i++) {
            this->_tokens.insert(
                this->_tokens.end(), this->_build_tokens[i].begin(), this->_build_tokens[i].end());
        }
        while (!this->_schema_element_stack.empty()) {
            this->_schema_element.targets.insert(this->_schema_element.targets.begin(),
                                                 this->_schema_element_stack.top());
            this->_schema_element_stack.pop();
        }
        this->_build_tokens.clear();
    }
}

const vector<string>& LinkSchema::tokens() {
    _check_frozen();
    return this->_tokens;
}

vector<string> LinkSchema::tokenize() {
    _check_frozen();
    return this->_tokens;
}

void LinkSchema::tokenize(vector<string>& output) {
    _check_frozen();
    for (string token : this->_tokens) {
        output.push_back(token);
    }
}

void LinkSchema::_syntax_assert(bool flag, string token, unsigned int cursor) {
    if (!flag) {
        if (cursor == 0) {
            cursor = 1;
        }
        Utils::error("Syntax error in LinkSchema untokenization near symbol `" + token +
                     "` near position " + std::to_string(cursor - 1));
    }
}

unsigned int LinkSchema::_push_stack_top(
    stack<tuple<string, string, unsigned int, unsigned int>>& link_schema_stack,
    string cursor_token,
    unsigned int cursor) {
    _syntax_assert(link_schema_stack.size() > 0, cursor_token, cursor);
    if (link_schema_stack.size() == 1) {
        return 0;
    }
    tuple<string, string, unsigned int, unsigned int> record = link_schema_stack.top();
    string token = get<0>(record);
    string type = get<1>(record);
    unsigned int arity = get<2>(record);
    unsigned int pending = get<3>(record);
    LOG_DEBUG("POP schema: " + token + " " + type + " " + std::to_string(arity) + " " +
              std::to_string(pending));
    if (token == LINK) {
        stack_link(type, arity);
    } else if (token == LINK_TEMPLATE) {
        stack_link_schema(type, arity);
    } else {
        _syntax_assert(false, cursor_token, cursor);
    }
    link_schema_stack.pop();
    if (--pending == 0) {
        LOG_DEBUG("Recursing _push_stack_top()");
        return _push_stack_top(link_schema_stack, cursor_token, cursor);
    } else {
        return pending;
    }
}

void LinkSchema::untokenize(const vector<string>& tokens) {
    if (_check_not_frozen()) {
        return;
    }

    stack<tuple<string, string, unsigned int, unsigned int>> link_schema_stack;
    string token, type, name, handle;
    unsigned int arity, current_pending_elements;
    unsigned int cursor = 0;
    token = tokens[cursor++];
    _syntax_assert(token == LINK_TEMPLATE, token, cursor);
    _syntax_assert((cursor + 2) < tokens.size(), token, cursor);
    type = tokens[cursor++];
    arity = (unsigned int) Utils::string_to_int(tokens[cursor++]);
    current_pending_elements = arity;
    LOG_DEBUG("PUSH schema: " << token << " " << type << " " << arity << " "
                              << current_pending_elements);
    link_schema_stack.push(make_tuple(token, type, arity, current_pending_elements));
    while (current_pending_elements > 0) {
        _syntax_assert((cursor + current_pending_elements) <= tokens.size(), token, cursor);
        token = tokens[cursor++];
        if (token == ATOM) {
            _syntax_assert((cursor + 1) <= tokens.size(), token, cursor);
            handle = tokens[cursor++];
            LOG_DEBUG("STACK ATOM: " << handle);
            stack_atom(handle);
            current_pending_elements--;
            if (current_pending_elements == 0) {
                current_pending_elements = _push_stack_top(link_schema_stack, token, cursor);
            }
        } else if (token == NODE) {
            _syntax_assert((cursor + 2) <= tokens.size(), token, cursor);
            type = tokens[cursor++];
            name = tokens[cursor++];
            LOG_DEBUG("STACK NODE: " << type << " " << name);
            stack_node(type, name);
            current_pending_elements--;
            if (current_pending_elements == 0) {
                current_pending_elements = _push_stack_top(link_schema_stack, token, cursor);
            }
        } else if (token == UNTYPED_VARIABLE) {
            _syntax_assert((cursor + 1) <= tokens.size(), token, cursor);
            name = tokens[cursor++];
            LOG_DEBUG("STACK UNTYPED_VARIABLE: " << name);
            stack_untyped_variable(name);
            current_pending_elements--;
            if (current_pending_elements == 0) {
                current_pending_elements = _push_stack_top(link_schema_stack, token, cursor);
            }
        } else if ((token == LINK) || (token == LINK_TEMPLATE)) {
            _syntax_assert((cursor + 2) <= tokens.size(), token, cursor);
            type = tokens[cursor++];
            arity = (unsigned int) Utils::string_to_int(tokens[cursor++]);
            LOG_DEBUG("STACK " << token << ": " << type << " " << arity);
            LOG_DEBUG("PUSH schema: " << token << " " << type << " " << arity << " "
                                      << current_pending_elements);
            link_schema_stack.push(make_tuple(token, type, arity, current_pending_elements));
            current_pending_elements = arity;
        } else {
            _syntax_assert(false, token, cursor);
        }
        LOG_DEBUG(std::to_string(this->_arity) << " " << std::to_string(this->_atom_stack.size()) << " "
                                               << std::to_string(link_schema_stack.size()));
        LOG_DEBUG("pending: " << current_pending_elements);
        if ((current_pending_elements == 0) && (link_schema_stack.size() == 1)) {
            LOG_DEBUG("BUILD");
            build();
        }
    }
}
