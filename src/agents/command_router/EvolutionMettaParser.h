#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "QueryAnswer.h"

using namespace std;
using namespace query_engine;

namespace command_router {

/** Canonical router / evolution param keys (also used in Properties). */
extern const string EVOLUTION_PARAM_QUERY;
extern const string EVOLUTION_PARAM_FITNESS;
extern const string EVOLUTION_PARAM_CORRELATION_QUERIES;
extern const string EVOLUTION_PARAM_CORRELATION_REPLACEMENTS;
extern const string EVOLUTION_PARAM_CORRELATION_MAPPINGS;

/** Fields parsed from a labeled MeTTa evolution ARG (context comes from router params). */
struct EvolutionMettaArgs {
    string query;
    string fitness_function_tag;
    /** One MeTTa S-expression per correlation query. */
    vector<string> correlation_query_expressions;
    /** Per-query replacement pairs (placeholder name, query-answer slot name). */
    vector<vector<pair<string, string>>> correlation_replacement_groups;
    /** Per-query mapping pairs (slot in original answer, slot in correlation answer). */
    vector<vector<pair<string, string>>> correlation_mapping_groups;
};

/**
 * Map evolution slot label or router param key to its canonical name.
 * Accepts: query; ff | fitness-function-tag; cq | correlation-queries;
 * cr | correlation-replacements; cm | correlation-mappings.
 * Returns empty string if unknown.
 */
string canonical_evolution_param_key(const string& key_or_alias);

/**
 * Parse evolution ARG as a MeTTa list of labeled clauses, e.g.:
 *   ((query (Contains $s1 (Word "bbb")))
 *    (ff count_letter)
 *    (cq ((Contains $p1 $w1) (Contains $p1 $w2)))
 *    (cr (((placeholder1 sentence1)) ((placeholder1 sentence1))))
 *    (cm (((sentence1 word1)) ((sentence1 word2)))))
 *
 * cq / cr / cm bodies use nested S-expressions (lists of queries or lists of (X Y) pairs).
 *
 * @return true if ARG uses the labeled form; false if ARG should be treated as a plain query.
 */
bool try_parse_evolution_metta_arg(const string& arg, EvolutionMettaArgs& out);

/** Parse cq body or correlation-queries router param (MeTTa list of query S-expressions). */
vector<string> parse_correlation_query_list_body(const string& body);

/**
 * Parse cr/cm body or router param (MeTTa list of groups; each group is a list of (X Y) pairs).
 * @throws std::runtime_error if body is not parenthesized MeTTa (e.g. key:value syntax).
 */
vector<vector<pair<string, string>>> parse_correlation_pair_groups_body(const string& body);

string normalize_metta_percent_variables(const string& expression);

vector<vector<string>> metta_correlation_queries(const vector<string>& expressions);

vector<map<string, QueryAnswerElement>> metta_correlation_replacements(
    const vector<vector<pair<string, string>>>& groups);

vector<vector<pair<QueryAnswerElement, QueryAnswerElement>>> metta_correlation_mappings(
    const vector<vector<pair<string, string>>>& groups);

}  // namespace command_router
