#include "Helper.h"
using namespace std;
using namespace mains;
using namespace commons;

bool Helper::is_running = true;
// Args names
string Helper::SERVICE = "service";
string Helper::ENDPOINT = "endpoint";
string Helper::BUS_ENDPOINT = "bus-endpoint";
string Helper::PORTS_RANGE = "ports-range";
string Helper::ATTENTION_BROKER_ADDRESS = "attention-broker-address";
string Helper::USE_MORK = "use-mork";
string Helper::ACTION = "action";
string Helper::TOKENS = "tokens";
string Helper::REQUEST = "request";
string Helper::TIMEOUT = "timeout";
string Helper::MAX_ANSWERS = "max-answers";
string Helper::ATTENTION_UPDATE_FLAG = "attention-update-flag";
string Helper::REPEAT_COUNT = "repeat-count";
string Helper::CONTEXT = "context";
string Helper::CORRELATION_QUERIES = "correlation-queries";
string Helper::CORRELATION_REPLACEMENTS = "correlation-replacements";
string Helper::CORRELATION_MAPPINGS = "correlation-mappings";
string Helper::FITNESS_FUNCTION_TAG = "fitness-function-tag";
string Helper::USE_CONTEXT_CACHE = "use-context-cache";
string Helper::ENFORCE_CACHE_RECREATION = "enforce-cache-recreation";
string Helper::INITIAL_RENT_RATE = "initial-rent-rate";
string Helper::INITIAL_SPREADING_RATE_LOWERBOUND = "initial-spreading-rate-lowerbound";
string Helper::INITIAL_SPREADING_RATE_UPPERBOUND = "initial-spreading-rate-upperbound";
string Helper::DETERMINER_SCHEMA = "determiner-schema";
string Helper::STIMULUS_SCHEMA = "stimulus-schema";
string Helper::POSITIVE_IMPORTANCE_FLAG = "positive-importance-flag";
string Helper::USE_METTA_AS_QUERY_TOKENS = "use-metta-as-query-tokens";
string Helper::UNIQUE_ASSIGNMENT_FLAG = "unique-assignment-flag";
string Helper::USE_LINK_TEMPLATE_CACHE = "use-link-template-cache";
string Helper::POPULATE_METTA_MAPPING = "populate-metta-mapping";
string Helper::QUERY = "query";

static map<ProcessorType, string> node_service_help = {{ProcessorType::INFERENCE_AGENT, string(R"(
Inference Agent:
This processor handles inference requests from the service bus.
Required arguments:
    - attention-broker-address: The address of the Attention Broker to connect to, in the form "host:port"
)")},
                                                       {ProcessorType::LINK_CREATION_AGENT, string(R"(
Link Creation Agent:
This processor manages link creation requests from the service bus.
)")},
                                                       {ProcessorType::CONTEXT_BROKER, string(R"(
Context Broker:
This processor handles context management requests from the service bus.
Required arguments:
    - attention-broker-address: The address of the Attention Broker to connect to, in the form "host:port"
)")},
                                                       {ProcessorType::EVOLUTION_AGENT, string(R"(
Evolution Agent:
This processor manages query evolution requests from the service bus.
Required arguments:
    - attention-broker-address: The address of the Attention Broker to connect to, in the form "host:port"
)")},
                                                       {ProcessorType::QUERY_ENGINE, string(R"(
Query Engine:
This processor handles pattern matching query requests from the service bus.
Required arguments:
    - attention-broker-address: The address of the Attention Broker to connect to, in the form "host:port"
)")},
                                                       {ProcessorType::ATOMDB_BROKER, string(R"(
AtomDB Broker:
This processor manages AtomDB broker requests from the service bus.
)")},
                                                       {ProcessorType::UNKNOWN, string(R"(
Usage:
busnode --service=<service> --hostname=<host:port> --ports_range=<start_port:end_port> [--peer_address=<peer_host:peer_port>] --attention-broker-address=<AB_ip:AB_port> [--use-mork=true|false]
)")}};

static map<ProcessorType, string> client_service_help = {{ProcessorType::INFERENCE_AGENT, string(R"(
Inference Agent Client:
This client sends inference requests to the Inference Agent via the service bus.
It requires the following arguments:
    - request: The inference request tokens.
 Optional arguments:
    - timeout: Timeout for the inference request in seconds
    - max-answers: Maximum number of answers to return
    - attention-update-flag: Whether to update the attention broker (true/false)
    - repeat-count: Number of times to repeat the request (0 for infinite)
)")},
                                                         {ProcessorType::LINK_CREATION_AGENT, string(R"(
Link Creation Agent Client:
This client sends link creation requests to the Link Creation Agent via the service bus.
It requires the following arguments:
    - request: The link creation request tokens.
 Optional arguments:
    - max-answers: Maximum number of answers to return
    - repeat-count: Number of times to repeat the request (0 for infinite)
    - context: Context for the link creation request
    - attention-update-flag: Whether to update the attention broker (true/false)
    - positive-importance-flag: Whether to set positive importance flag (true/false)
    - use-metta-as-query-tokens: Whether to use MeTTa expressions as query tokens (true/false)
)")},
                                                         {ProcessorType::CONTEXT_BROKER, string(R"(
Context Broker Client:
This client sends context management requests to the Context Broker via the service bus.
It requires the following arguments:
    - context: The context in which the query should be evaluated.
    - query: The query to be processed.
    - determiner-schema: The determiner schema for the query.
    - stimulus-schema: The stimulus schema for the query.
 Optional arguments:
    - use-context-cache: Whether to use the context cache (true/false)
    - enforce-cache-recreation: Whether to enforce cache recreation (true/false)
    - initial-rent-rate: The initial rent rate for the context
    - initial-spreading-rate-lowerbound: The initial spreading rate lower bound
    - initial-spreading-rate-upperbound: The initial spreading rate upper bound
)")},
                                                         {ProcessorType::EVOLUTION_AGENT, string(R"(
Evolution Agent Client:
This client sends query evolution requests to the Evolution Agent via the service bus.
It requires the following arguments:
    - query: The query to be processed.
    - correlation-queries: The correlation queries associated with the evolution request.
    - correlation-replacements: The correlation replacements for the evolution request.
    - correlation-mappings: The correlation mappings for the evolution request.
    - context: The context in which the evolution should be evaluated.
    - fitness-function-tag: The fitness function tag to be used.
)")},
                                                         {ProcessorType::QUERY_ENGINE, string(R"(
Query Engine Client:
This client sends pattern matching query requests to the Query Engine via the service bus.
It requires the following arguments:
    - query: The query to be processed.
    - context: The context in which the query should be evaluated.
 Optional arguments:
    - unique-assignment-flag: Whether to enforce unique assignments (true/false)
    - attention-update-flag: Whether to update the attention broker (true/false)
    - max-answers: Maximum number of answers to return
    - use-link-template-cache: Whether to use link template cache (true/false)
    - populate-metta-mapping: Whether to populate MeTTa mapping (true/false)
    - use-metta-as-query-tokens: Whether to use MeTTa expressions as query tokens (true/false)
    - positive-importance-flag: Whether to set positive importance flag (true/false)
)")},
                                                         {ProcessorType::ATOMDB_BROKER, string(R"(
AtomDB Broker Client:
This client interacts with the AtomDB Broker via the service bus.
 It requires the following arguments:
    - action: The action to be performed, supported actions: (add_atoms).
    - tokens: The tokens associated with the action.
 Optional arguments:
    - use-mork: Whether to use MorkDB as the backend (true/false)
)")},
                                                         {ProcessorType::UNKNOWN, string(R"(
Usage:
busclient --service=<service> --hostname=<host:port> --service-hostname=<service_host:service_port> --ports-range=<start_port:end_port> [--use-mork=true|false]
        )")}};

static map<string, ProcessorType> string_to_processor_type = {
    {"inference-agent", ProcessorType::INFERENCE_AGENT},
    {"link-creation-agent", ProcessorType::LINK_CREATION_AGENT},
    {"context-broker", ProcessorType::CONTEXT_BROKER},
    {"evolution-agent", ProcessorType::EVOLUTION_AGENT},
    {"query-engine", ProcessorType::QUERY_ENGINE},
    {"atomdb-broker", ProcessorType::ATOMDB_BROKER}};

string Helper::help(const ProcessorType& processor_type, ServiceCallerType caller_type) {
    string usage;
    if (caller_type == ServiceCallerType::CLIENT) {
        usage = client_service_help[ProcessorType::UNKNOWN];
    } else {
        usage = node_service_help[ProcessorType::UNKNOWN];
    }
    switch (processor_type) {
        case ProcessorType::INFERENCE_AGENT:
            if (caller_type == ServiceCallerType::CLIENT) {
                return usage + client_service_help[ProcessorType::INFERENCE_AGENT];
            } else {
                return usage + node_service_help[ProcessorType::INFERENCE_AGENT];
            }
        case ProcessorType::LINK_CREATION_AGENT:
            if (caller_type == ServiceCallerType::CLIENT) {
                return usage + client_service_help[ProcessorType::LINK_CREATION_AGENT];
            } else {
                return usage + node_service_help[ProcessorType::LINK_CREATION_AGENT];
            }
        case ProcessorType::CONTEXT_BROKER:
            if (caller_type == ServiceCallerType::CLIENT) {
                return usage + client_service_help[ProcessorType::CONTEXT_BROKER];
            } else {
                return usage + node_service_help[ProcessorType::CONTEXT_BROKER];
            }
        case ProcessorType::EVOLUTION_AGENT:
            if (caller_type == ServiceCallerType::CLIENT) {
                return usage + client_service_help[ProcessorType::EVOLUTION_AGENT];
            } else {
                return usage + node_service_help[ProcessorType::EVOLUTION_AGENT];
            }
        case ProcessorType::QUERY_ENGINE:
            if (caller_type == ServiceCallerType::CLIENT) {
                return usage + client_service_help[ProcessorType::QUERY_ENGINE];
            } else {
                return usage + node_service_help[ProcessorType::QUERY_ENGINE];
            }
        case ProcessorType::ATOMDB_BROKER:
            if (caller_type == ServiceCallerType::CLIENT) {
                return usage + client_service_help[ProcessorType::ATOMDB_BROKER];
            } else {
                return usage + node_service_help[ProcessorType::ATOMDB_BROKER];
            }
        default:
            vector<string> avaiable_services;
            for (const auto& service : string_to_processor_type) {
                avaiable_services.push_back(" - " + service.first);
            }
            return string(usage) + string("\nAvaiable services:\n") +
                   Utils::join(avaiable_services, '\n');
    }
}

vector<string> Helper::get_required_arguments(const string& processor_type,
                                              ServiceCallerType caller_type) {
    ProcessorType p_type = processor_type_from_string(processor_type);
    switch (p_type) {
        case ProcessorType::INFERENCE_AGENT:
            if (caller_type == ServiceCallerType::CLIENT) {
                return {REQUEST};
            } else {
                return {ATTENTION_BROKER_ADDRESS};
            }
        case ProcessorType::LINK_CREATION_AGENT:
            if (caller_type == ServiceCallerType::CLIENT) {
                return {REQUEST};
            } else {
                return {};
            }
        case ProcessorType::CONTEXT_BROKER:
            if (caller_type == ServiceCallerType::CLIENT) {
                return {CONTEXT,
                        QUERY,
                        DETERMINER_SCHEMA,
                        STIMULUS_SCHEMA,
                        USE_CONTEXT_CACHE,
                        ENFORCE_CACHE_RECREATION};
            } else {
                return {ATTENTION_BROKER_ADDRESS};
            }
        case ProcessorType::EVOLUTION_AGENT:
            if (caller_type == ServiceCallerType::CLIENT) {
                return {REQUEST, MAX_ANSWERS, ATTENTION_UPDATE_FLAG, REPEAT_COUNT};
            } else {
                return {ATTENTION_BROKER_ADDRESS};
            }
        case ProcessorType::QUERY_ENGINE:
            if (caller_type == ServiceCallerType::CLIENT) {
                return {QUERY, CONTEXT};
            } else {
                return {ATTENTION_BROKER_ADDRESS};
            }
        case ProcessorType::ATOMDB_BROKER:
            if (caller_type == ServiceCallerType::CLIENT) {
                return {ACTION, TOKENS};
            } else {
                return {};
            }
        default:
            return {};
    }
}

ProcessorType Helper::processor_type_from_string(const string& type_str) {
    if (string_to_processor_type.find(type_str) != string_to_processor_type.end()) {
        return string_to_processor_type[type_str];
    } else {
        return ProcessorType::UNKNOWN;
    }
}