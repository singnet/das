#pragma once

#include <string>

using namespace std;

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
    string correlation_queries;
    string correlation_replacements;
    string correlation_mappings;
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
 *    (cq (Contains $p1 $w1))
 *    (cr $p1:s1)
 *    (cm s1:$w1))
 *
 * Full names and aliases may be mixed in one ARG.
 *
 * @return true if ARG uses the labeled form; false if ARG should be treated as a plain query.
 */
bool try_parse_evolution_metta_arg(const string& arg, EvolutionMettaArgs& out);

}  // namespace command_router
