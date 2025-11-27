#pragma once

#include "Utils.h"

using namespace std;

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

    // Args names
    static string SERVICE;
    static string ENDPOINT;
    static string BUS_ENDPOINT;
    static string PORTS_RANGE;
    static string ATTENTION_BROKER_ENDPOINT;
    static string USE_MORK;
    static string ACTION;
    static string TOKENS;
    static string REQUEST;
    static string TIMEOUT;
    static string MAX_ANSWERS;
    static string ATTENTION_UPDATE_FLAG;
    static string REPEAT_COUNT;
    static string CONTEXT;
    static string CORRELATION_QUERIES;
    static string CORRELATION_REPLACEMENTS;
    static string CORRELATION_MAPPINGS;
    static string FITNESS_FUNCTION_TAG;
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

    static bool is_running;

    static string help(const ProcessorType& processor_type,
                       ServiceCallerType caller_type = ServiceCallerType::UNKNOWN);

    static vector<string> get_required_arguments(
        const string& processor_type, ServiceCallerType caller_type = ServiceCallerType::UNKNOWN);

    static ProcessorType processor_type_from_string(const string& type_str);
};
}  // namespace mains