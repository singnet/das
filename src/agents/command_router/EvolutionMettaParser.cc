#include "EvolutionMettaParser.h"

#include <unordered_map>

#include "Utils.h"

using namespace command_router;
using namespace commons;

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

void skip_space(const string& s, size_t& pos) {
    while (pos < s.size() && isspace(static_cast<unsigned char>(s[pos]))) {
        pos++;
    }
}

bool extract_balanced(const string& s, size_t& pos, string& out) {
    skip_space(s, pos);
    if (pos >= s.size() || s[pos] != '(') {
        return false;
    }
    size_t start = pos;
    int depth = 0;
    do {
        if (s[pos] == '"') {
            pos++;
            while (pos < s.size() && s[pos] != '"') {
                if (s[pos] == '\\' && pos + 1 < s.size()) {
                    pos += 2;
                } else {
                    pos++;
                }
            }
            if (pos < s.size()) {
                pos++;
            }
            continue;
        }
        if (s[pos] == '(') {
            depth++;
        } else if (s[pos] == ')') {
            depth--;
        }
        pos++;
    } while (pos < s.size() && depth > 0);
    if (depth != 0) {
        return false;
    }
    out = s.substr(start, pos - start);
    return true;
}

vector<string> top_level_expressions(const string& s) {
    vector<string> children;
    size_t pos = 0;
    skip_space(s, pos);
    if (pos >= s.size() || s[pos] != '(') {
        return children;
    }
    pos++;
    while (pos < s.size()) {
        skip_space(s, pos);
        if (pos < s.size() && s[pos] == ')') {
            break;
        }
        string element;
        if (!extract_balanced(s, pos, element)) {
            size_t start = pos;
            while (pos < s.size() && !isspace(static_cast<unsigned char>(s[pos])) && s[pos] != ')') {
                pos++;
            }
            if (pos > start) {
                children.push_back(s.substr(start, pos - start));
            }
            continue;
        }
        children.push_back(element);
    }
    return children;
}

bool parse_labeled_clause(const string& clause, string& label, string& body) {
    vector<string> children = top_level_expressions(clause);
    if (children.empty()) {
        return false;
    }
    label = children[0];
    body = "";
    for (size_t i = 1; i < children.size(); i++) {
        if (i > 1) body += " ";
        body += children[i];
    }
    Utils::trim(body);
    return !label.empty();
}

string unwrap_single_child_group(string group_clause) {
    while (true) {
        vector<string> children = top_level_expressions(group_clause);
        if (children.size() == 1 && !children[0].empty() && children[0][0] == '(') {
            group_clause = children[0];
            continue;
        }
        return group_clause;
    }
}

vector<pair<string, string>> parse_pairs_from_group(const string& group_clause) {
    vector<pair<string, string>> pairs;
    string inner = unwrap_single_child_group(group_clause);
    vector<string> elements = top_level_expressions(inner);
    if (elements.size() >= 2 && !elements[0].empty() && elements[0][0] != '(') {
        pairs.emplace_back(elements[0], elements[1]);
        return pairs;
    }
    for (const auto& pair_clause : elements) {
        vector<string> atoms = top_level_expressions(pair_clause);
        if (atoms.size() >= 2 && !atoms[0].empty() && atoms[0][0] != '(') {
            pairs.emplace_back(atoms[0], atoms[1]);
        }
    }
    return pairs;
}

vector<vector<pair<string, string>>> parse_pair_groups_body(const string& body) {
    vector<vector<pair<string, string>>> groups;
    string inner_body = unwrap_single_child_group(body);
    vector<string> elements = top_level_expressions(inner_body);
    if (elements.size() >= 2 && !elements[0].empty() && elements[0][0] != '(') {
        groups.push_back({make_pair(elements[0], elements[1])});
        return groups;
    }
    for (const auto& group_clause : elements) {
        groups.push_back(parse_pairs_from_group(group_clause));
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
    if (body.empty()) {
        return {};
    }
    if (body[0] != '(') {
        return {body};
    }
    vector<string> queries = top_level_expressions(body);
    if (queries.empty()) {
        return {body};
    }
    if (queries.size() == 1) {
        return queries;
    }
    // List of query S-expressions: ((Q1) (Q2)); not a single query tokenized as Contains, %v, ...
    if (queries[0][0] == '(') {
        return queries;
    }
    return {body};
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
    return parse_pair_groups_body(trimmed);
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

vector<vector<pair<QueryAnswerElement, QueryAnswerElement>>>
command_router::metta_correlation_mappings(const vector<vector<pair<string, string>>>& groups) {
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

    vector<string> clauses = top_level_expressions(trimmed);
    if (clauses.empty()) {
        return false;
    }

    bool found_labeled_slot = false;
    for (const auto& clause : clauses) {
        string label;
        string body;
        if (!parse_labeled_clause(clause, label, body)) {
            continue;
        }
        string canonical = canonical_evolution_param_key(label);
        if (canonical.empty()) {
            continue;
        }
        found_labeled_slot = true;
        if (canonical == PARAM_QUERY) {
            out.query = body;
        } else if (canonical == PARAM_FITNESS_FUNCTION) {
            out.fitness_function_tag = body;
        } else if (canonical == PARAM_CORRELATION_QUERIES) {
            out.correlation_query_expressions = parse_correlation_query_list_body(body);
        } else if (canonical == PARAM_CORRELATION_REPLACEMENTS) {
            out.correlation_replacement_groups = parse_correlation_pair_groups_body(body);
        } else if (canonical == PARAM_CORRELATION_MAPPINGS) {
            out.correlation_mapping_groups = parse_correlation_pair_groups_body(body);
        }
    }

    if (clauses.size() > 1) {
        return found_labeled_slot && !out.query.empty();
    }
    return found_labeled_slot;
}
