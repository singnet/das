#include "EvolutionMettaParser.h"

#include <algorithm>
#include <stack>
#include <unordered_map>

#include "Atom.h"
#include "Link.h"
#include "MettaMapping.h"
#include "MettaParser.h"
#include "Node.h"
#include "ParserActions.h"
#include "UntypedVariable.h"
#include "Utils.h"

using namespace command_router;
using namespace commons;
using namespace atoms;

const string command_router::PARAM_QUERY = "query";
const string command_router::PARAM_FITNESS_FUNCTION = "fitness-function-tag";
const string command_router::PARAM_CORRELATION_QUERIES = "correlation-queries";
const string command_router::PARAM_CORRELATION_REPLACEMENTS = "correlation-replacements";
const string command_router::PARAM_CORRELATION_MAPPINGS = "correlation-mappings";

namespace {

const string ALIAS_QUERY = "q";
const string ALIAS_FITNESS_FUNCTION = "ff";
const string ALIAS_CORRELATION_QUERIES = "cq";
const string ALIAS_CORRELATION_REPLACEMENTS = "cr";
const string ALIAS_CORRELATION_MAPPINGS = "cm";

const unordered_map<string, string>& param_key_aliases() {
    static const unordered_map<string, string> aliases = {
        {PARAM_QUERY, PARAM_QUERY},
        {ALIAS_QUERY, PARAM_QUERY},
        {PARAM_FITNESS_FUNCTION, PARAM_FITNESS_FUNCTION},
        {ALIAS_FITNESS_FUNCTION, PARAM_FITNESS_FUNCTION},
        {PARAM_CORRELATION_QUERIES, PARAM_CORRELATION_QUERIES},
        {ALIAS_CORRELATION_QUERIES, PARAM_CORRELATION_QUERIES},
        {PARAM_CORRELATION_REPLACEMENTS, PARAM_CORRELATION_REPLACEMENTS},
        {ALIAS_CORRELATION_REPLACEMENTS, PARAM_CORRELATION_REPLACEMENTS},
        {PARAM_CORRELATION_MAPPINGS, PARAM_CORRELATION_MAPPINGS},
        {ALIAS_CORRELATION_MAPPINGS, PARAM_CORRELATION_MAPPINGS},
    };
    return aliases;
}

/**
 * Custom MeTTa ParserActions for evolution ARG: builds Node/Link/UntypedVariable
 * atoms without the AND/OR special-casing done by commons/atoms/MettaParserActions
 * (queries inside the ARG may legitimately use `and`/`or` as plain symbols),
 * and accepts the legacy `%name` variable syntax in addition to `$name`.
 */
class EvolutionParserActions : public metta::ParserActions {
   public:
    void symbol(const string& name) override {
        if (name.size() > 1 && name[0] == '%') {
            push_variable(name.substr(1), name);
        } else {
            push_node(name);
        }
    }

    void variable(const string& value) override { push_variable(value, "$" + value); }

    void literal(const string& value) override { push_node(value); }

    void literal(int value) override { push_node(std::to_string(value)); }

    void literal(float value) override { push_node(std::to_string(value)); }

    void expression_begin() override {
        this->expression_size_stack.push(this->current_expression_size);
        this->current_expression_size = 0;
    }

    void expression_end(bool toplevel, const string& metta_string) override {
        unsigned int arity = this->current_expression_size;
        if (this->element_stack.size() < arity) {
            RAISE_ERROR("Invalid metta expression: too few arguments");
        }
        vector<shared_ptr<Atom>> children(arity);
        for (unsigned int i = 0; i < arity; i++) {
            children[arity - 1 - i] = this->element_stack.top();
            this->element_stack.pop();
        }
        vector<string> target_handles;
        target_handles.reserve(arity);
        for (const auto& child : children) {
            target_handles.push_back(child->handle());
        }
        auto link = make_shared<Link>(MettaMapping::EXPRESSION_LINK_TYPE, target_handles, toplevel);
        this->handle_to_atom[link->handle()] = link;
        this->handle_to_metta_expression[link->handle()] = metta_string;
        this->element_stack.push(link);
        if (toplevel) {
            this->root_handle = link->handle();
        }
        this->current_expression_size = this->expression_size_stack.top() + 1;
        this->expression_size_stack.pop();
    }

    string root_handle;
    map<string, shared_ptr<Atom>> handle_to_atom;
    map<string, string> handle_to_metta_expression;

   private:
    void push_node(const string& name) {
        auto node = make_shared<Node>(MettaMapping::SYMBOL_NODE_TYPE, name);
        this->handle_to_atom[node->handle()] = node;
        this->handle_to_metta_expression[node->handle()] = name;
        this->element_stack.push(node);
        this->current_expression_size++;
    }

    void push_variable(const string& name, const string& source_text) {
        auto variable = make_shared<UntypedVariable>(name);
        this->handle_to_atom[variable->handle()] = variable;
        this->handle_to_metta_expression[variable->handle()] = source_text;
        this->element_stack.push(variable);
        this->current_expression_size++;
    }

    stack<shared_ptr<Atom>> element_stack;
    stack<unsigned int> expression_size_stack;
    unsigned int current_expression_size = 0;
};

shared_ptr<EvolutionParserActions> parse_metta(const string& metta_string) {
    auto actions = make_shared<EvolutionParserActions>();
    metta::MettaParser parser(metta_string, actions);
    parser.parse();
    return actions;
}

shared_ptr<Link> as_link(const shared_ptr<Atom>& atom) { return dynamic_pointer_cast<Link>(atom); }

string atom_name(const shared_ptr<Atom>& atom) {
    if (auto node = dynamic_pointer_cast<Node>(atom)) {
        return node->name;
    }
    if (auto variable = dynamic_pointer_cast<UntypedVariable>(atom)) {
        return variable->name;
    }
    return "";
}

vector<shared_ptr<Atom>> link_targets(const shared_ptr<Link>& link,
                                      const EvolutionParserActions& actions) {
    vector<shared_ptr<Atom>> atoms;
    atoms.reserve(link->targets.size());
    for (const auto& handle : link->targets) {
        auto iter = actions.handle_to_atom.find(handle);
        if (iter == actions.handle_to_atom.end()) {
            RAISE_ERROR("Internal parser error: unknown atom handle");
        }
        atoms.push_back(iter->second);
    }
    return atoms;
}

vector<string> walk_query_list(const shared_ptr<Atom>& body_atom,
                               const EvolutionParserActions& actions) {
    auto body_link = as_link(body_atom);
    if (!body_link) {
        return {};
    }
    vector<shared_ptr<Atom>> targets = link_targets(body_link, actions);
    if (!targets.empty() && Atom::is_link(targets[0])) {
        vector<string> queries;
        queries.reserve(targets.size());
        for (const auto& target : targets) {
            queries.push_back(actions.handle_to_metta_expression.at(target->handle()));
        }
        return queries;
    }
    return {actions.handle_to_metta_expression.at(body_link->handle())};
}

bool is_atom_pair_link(const shared_ptr<Link>& link, const EvolutionParserActions& actions) {
    if (link->arity() != 2) {
        return false;
    }
    for (const auto& target : link_targets(link, actions)) {
        if (Atom::is_link(target)) {
            return false;
        }
    }
    return true;
}

pair<string, string> as_pair(const shared_ptr<Link>& link, const EvolutionParserActions& actions) {
    if (!is_atom_pair_link(link, actions)) {
        RAISE_ERROR("Expected pair (X Y) of two atoms");
    }
    auto targets = link_targets(link, actions);
    return make_pair(atom_name(targets[0]), atom_name(targets[1]));
}

vector<vector<pair<string, string>>> walk_pair_groups(const shared_ptr<Atom>& body_atom,
                                                      const EvolutionParserActions& actions) {
    auto body_link = as_link(body_atom);
    if (!body_link) {
        RAISE_ERROR(
            "correlation-replacements and correlation-mappings require a 3-level MeTTa form: "
            "(((X Y) ...) ...) - list of groups of (X Y) pairs");
    }
    vector<vector<pair<string, string>>> groups;
    for (const auto& group_atom : link_targets(body_link, actions)) {
        auto group_link = as_link(group_atom);
        if (!group_link) {
            RAISE_ERROR(
                "Each correlation group must be an S-expression list of pairs, "
                "e.g. ((X1 Y1) (X2 Y2))");
        }
        vector<pair<string, string>> pairs;
        for (const auto& pair_atom : link_targets(group_link, actions)) {
            auto pair_link = as_link(pair_atom);
            if (!pair_link) {
                RAISE_ERROR(
                    "Each correlation entry must be a pair S-expression (X Y); "
                    "a single pair as a group requires triple-wrapping, e.g. (((X Y)))");
            }
            pairs.push_back(as_pair(pair_link, actions));
        }
        groups.push_back(pairs);
    }
    return groups;
}

string strip_leading_variable_sigil(const string& name) {
    if (!name.empty() && (name[0] == '%' || name[0] == '$')) {
        return name.substr(1);
    }
    return name;
}

}  // namespace

string command_router::canonical_evolution_param_key(const string& key_or_alias) {
    auto iterator = param_key_aliases().find(key_or_alias);
    if (iterator == param_key_aliases().end()) {
        return "";
    }
    return iterator->second;
}

string command_router::normalize_metta_percent_variables(const string& expression) {
    string parsed = expression;
    Utils::replace_all(parsed, "%", "$");
    return parsed;
}

vector<string> command_router::parse_correlation_query_list_body(const string& body) {
    string trimmed = body;
    Utils::trim(trimmed);
    if (trimmed.empty()) {
        return {};
    }
    if (trimmed[0] != '(') {
        return {trimmed};
    }
    auto actions = parse_metta(trimmed);
    if (actions->root_handle.empty()) {
        return {trimmed};
    }
    return walk_query_list(actions->handle_to_atom.at(actions->root_handle), *actions);
}

vector<vector<pair<string, string>>> command_router::parse_correlation_pair_groups_body(
    const string& body) {
    string trimmed = body;
    Utils::trim(trimmed);
    if (trimmed.empty()) {
        return {};
    }
    if (trimmed[0] != '(') {
        RAISE_ERROR(
            "correlation-replacements and correlation-mappings require MeTTa S-expressions "
            "(e.g. ((placeholder1 sentence1))); key:value syntax is not supported");
    }
    auto actions = parse_metta(trimmed);
    if (actions->root_handle.empty()) {
        return {};
    }
    return walk_pair_groups(actions->handle_to_atom.at(actions->root_handle), *actions);
}

vector<vector<string>> command_router::metta_correlation_queries(const vector<string>& expressions) {
    vector<vector<string>> queries;
    for (const auto& expression : expressions) {
        queries.push_back({normalize_metta_percent_variables(expression)});
    }
    return queries;
}

vector<map<string, QueryAnswerElement>> command_router::metta_correlation_replacements(
    const vector<vector<pair<string, string>>>& groups) {
    vector<map<string, QueryAnswerElement>> replacements;
    for (const auto& group : groups) {
        map<string, QueryAnswerElement> replacement_map;
        for (const auto& pair : group) {
            string key = strip_leading_variable_sigil(normalize_metta_percent_variables(pair.first));
            string value = strip_leading_variable_sigil(normalize_metta_percent_variables(pair.second));
            replacement_map[key] = QueryAnswerElement(value);
        }
        replacements.push_back(replacement_map);
    }
    return replacements;
}

vector<vector<pair<QueryAnswerElement, QueryAnswerElement>>> command_router::metta_correlation_mappings(
    const vector<vector<pair<string, string>>>& groups) {
    vector<vector<pair<QueryAnswerElement, QueryAnswerElement>>> mappings;
    for (const auto& group : groups) {
        vector<pair<QueryAnswerElement, QueryAnswerElement>> mapping;
        for (const auto& pair : group) {
            string first = strip_leading_variable_sigil(normalize_metta_percent_variables(pair.first));
            string second = strip_leading_variable_sigil(normalize_metta_percent_variables(pair.second));
            mapping.push_back(make_pair(QueryAnswerElement(first), QueryAnswerElement(second)));
        }
        mappings.push_back(mapping);
    }
    return mappings;
}

bool command_router::try_parse_evolution_metta_arg(const string& arg, EvolutionMettaArgs& out) {
    out = EvolutionMettaArgs{};
    string trimmed = arg;
    Utils::trim(trimmed);
    if (trimmed.empty() || trimmed[0] != '(') {
        return false;
    }

    auto actions = parse_metta(trimmed);
    if (actions->root_handle.empty()) {
        return false;
    }
    auto root_link = as_link(actions->handle_to_atom.at(actions->root_handle));
    if (!root_link) {
        return false;
    }

    bool found_labeled_slot = false;
    vector<shared_ptr<Atom>> clauses = link_targets(root_link, *actions);
    for (const auto& clause_atom : clauses) {
        auto clause_link = as_link(clause_atom);
        if (!clause_link) {
            continue;
        }
        vector<shared_ptr<Atom>> children = link_targets(clause_link, *actions);
        if (children.empty()) {
            continue;
        }
        string canonical = canonical_evolution_param_key(atom_name(children[0]));
        if (canonical.empty()) {
            continue;
        }
        found_labeled_slot = true;
        if (children.size() < 2) {
            continue;
        }
        const auto& body = children[1];
        if (canonical == PARAM_QUERY) {
            out.query = actions->handle_to_metta_expression.at(body->handle());
        } else if (canonical == PARAM_FITNESS_FUNCTION) {
            out.fitness_function_tag = atom_name(body);
        } else if (canonical == PARAM_CORRELATION_QUERIES) {
            out.correlation_query_expressions = walk_query_list(body, *actions);
        } else if (canonical == PARAM_CORRELATION_REPLACEMENTS) {
            out.correlation_replacement_groups = walk_pair_groups(body, *actions);
        } else if (canonical == PARAM_CORRELATION_MAPPINGS) {
            out.correlation_mapping_groups = walk_pair_groups(body, *actions);
        }
    }

    if (clauses.size() > 1) {
        return found_labeled_slot && !out.query.empty();
    }
    return found_labeled_slot;
}
