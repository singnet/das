#include "EvolutionMettaParser.h"

#include <unordered_map>

#include "Utils.h"

using namespace command_router;
using namespace commons;

const string command_router::EVOLUTION_PARAM_QUERY = "query";
const string command_router::EVOLUTION_PARAM_FITNESS = "fitness-function-tag";
const string command_router::EVOLUTION_PARAM_CORRELATION_QUERIES = "correlation-queries";
const string command_router::EVOLUTION_PARAM_CORRELATION_REPLACEMENTS = "correlation-replacements";
const string command_router::EVOLUTION_PARAM_CORRELATION_MAPPINGS = "correlation-mappings";

namespace {

const string ALIAS_FITNESS = "ff";
const string ALIAS_CORRELATION_QUERIES = "cq";
const string ALIAS_CORRELATION_REPLACEMENTS = "cr";
const string ALIAS_CORRELATION_MAPPINGS = "cm";

const unordered_map<string, string>& param_key_aliases() {
    static const unordered_map<string, string> aliases = {
        {EVOLUTION_PARAM_QUERY, EVOLUTION_PARAM_QUERY},
        {EVOLUTION_PARAM_FITNESS, EVOLUTION_PARAM_FITNESS},
        {ALIAS_FITNESS, EVOLUTION_PARAM_FITNESS},
        {EVOLUTION_PARAM_CORRELATION_QUERIES, EVOLUTION_PARAM_CORRELATION_QUERIES},
        {ALIAS_CORRELATION_QUERIES, EVOLUTION_PARAM_CORRELATION_QUERIES},
        {EVOLUTION_PARAM_CORRELATION_REPLACEMENTS, EVOLUTION_PARAM_CORRELATION_REPLACEMENTS},
        {ALIAS_CORRELATION_REPLACEMENTS, EVOLUTION_PARAM_CORRELATION_REPLACEMENTS},
        {EVOLUTION_PARAM_CORRELATION_MAPPINGS, EVOLUTION_PARAM_CORRELATION_MAPPINGS},
        {ALIAS_CORRELATION_MAPPINGS, EVOLUTION_PARAM_CORRELATION_MAPPINGS},
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
    if (clause.empty() || clause[0] != '(') {
        return false;
    }
    string inner = clause.substr(1);
    if (!inner.empty() && inner.back() == ')') {
        inner.pop_back();
    }
    Utils::trim(inner);
    size_t label_end = inner.find(' ');
    if (label_end == string::npos) {
        label = inner;
        body = "";
    } else {
        label = inner.substr(0, label_end);
        body = inner.substr(label_end + 1);
        Utils::trim(label);
        Utils::trim(body);
    }
    return !label.empty();
}

void assign_slot(EvolutionMettaArgs& out, const string& canonical, const string& body) {
    if (canonical == EVOLUTION_PARAM_QUERY) {
        out.query = body;
    } else if (canonical == EVOLUTION_PARAM_FITNESS) {
        out.fitness_function_tag = body;
    } else if (canonical == EVOLUTION_PARAM_CORRELATION_QUERIES) {
        out.correlation_queries = body;
    } else if (canonical == EVOLUTION_PARAM_CORRELATION_REPLACEMENTS) {
        out.correlation_replacements = body;
    } else if (canonical == EVOLUTION_PARAM_CORRELATION_MAPPINGS) {
        out.correlation_mappings = body;
    }
}

}  // namespace

string command_router::canonical_evolution_param_key(const string& key_or_alias) {
    auto iterator = param_key_aliases().find(key_or_alias);
    if (iterator == param_key_aliases().end()) {
        return "";
    }
    return iterator->second;
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
        assign_slot(out, canonical, body);
    }

    if (clauses.size() > 1) {
        return found_labeled_slot && !out.query.empty();
    }
    if (clauses.size() == 1) {
        string label;
        string body;
        if (!parse_labeled_clause(clauses[0], label, body)) {
            return false;
        }
        return !canonical_evolution_param_key(label).empty();
    }
    return false;
}
