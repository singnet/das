#pragma once

#include "JsonConfig.h"
#include "Utils.h"

using namespace std;
using namespace commons;

namespace mains {

enum class ProcessorType {
    INFERENCE_AGENT,
    LINK_CREATION_AGENT,
    CONTEXT_BROKER,
    EVOLUTION_AGENT,
    QUERY_ENGINE,
    ATOMDB_BROKER,
    UNKNOWN
};

enum class ServiceCallerType { CLIENT, NODE, UNKNOWN };

class Helper {
   public:
    Helper() = default;
    ~Helper() = default;

    // Args names (CLI keys). bus_client: required --client; SERVICE is derived (not a CLI flag).
    // bus_node vs bus_client use ENDPOINT / BUS_ENDPOINT differently; see merge_client_json_defaults.
    static string SERVICE;
    /** Agent/broker key in `das.json` (e.g. query -> agents.query.endpoint). */
    static string CLIENT;
    /** bus_node: this node's listen address. bus_client: remote bus peer id (ServiceBus known_peer). */
    static string ENDPOINT;
    /** bus_client: this client's listen address on the bus (ServiceBus host_id). bus_node: optional
     * peer. */
    static string BUS_ENDPOINT;
    static string PORTS_RANGE;
    static string ATTENTION_BROKER_ENDPOINT;
    static string USE_MORK;
    static string ACTION;
    static string TOKENS;
    static string REQUEST;
    static string TIMEOUT;
    static string MAX_ANSWERS;
    static string ATTENTION_UPDATE;
    static string ATTENTION_CORRELATION;
    static string REPEAT_COUNT;
    static string CONTEXT;
    static string CORRELATION_QUERIES;
    static string CORRELATION_REPLACEMENTS;
    static string CORRELATION_MAPPINGS;
    static string FITNESS_FUNCTION_TAG;
    static string POPULATION_SIZE;
    static string MAX_GENERATIONS;
    static string ELITISM_RATE;
    static string SELECTION_RATE;
    static string TOTAL_ATTENTION_TOKENS;
    static string USE_CONTEXT_CACHE;
    static string ENFORCE_CACHE_RECREATION;
    static string INITIAL_RENT_RATE;
    static string INITIAL_SPREADING_RATE_LOWERBOUND;
    static string INITIAL_SPREADING_RATE_UPPERBOUND;
    static string DETERMINER_SCHEMA;
    static string STIMULUS_SCHEMA;
    static string POSITIVE_IMPORTANCE_FLAG;
    static string USE_METTA_AS_QUERY_TOKENS;
    static string UNIQUE_ASSIGNMENT_FLAG;
    static string USE_LINK_TEMPLATE_CACHE;
    static string POPULATE_METTA_MAPPING;
    static string QUERY;

    /** Maps CLI arg names to dotted JSON config paths (e.g. "query-engine" -> "agents.query"). */
    static map<string, string> arg_to_json_config_key;
    /** Merges params from client json config to cmd_args. */
    static void merge_params_from_client_json_config(map<string, string>& cmd_args,
                                                     JsonConfig& json_config);

    static bool is_running;

    static string help(const ProcessorType& processor_type,
                       ServiceCallerType caller_type = ServiceCallerType::UNKNOWN);

    static vector<string> get_required_arguments(
        const string& processor_type, ServiceCallerType caller_type = ServiceCallerType::UNKNOWN);

    static ProcessorType processor_type_from_string(const string& type_str);
};
}  // namespace mains
