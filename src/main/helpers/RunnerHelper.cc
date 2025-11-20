#include "RunnerHelper.h"
using namespace std;
using namespace mains;

bool RunnerHelper::is_running = true;

static map<ProcessorType, string> node_service_help = {{ProcessorType::INFERENCE_AGENT, string(R"(
Inference Agent:
This processor handles inference requests from the service bus.
Required arguments:
    - attention_broker_address: The address of the Attention Broker to connect to, in the form "host:port"
)")},
                                                       {ProcessorType::LINK_CREATION_AGENT, string(R"(
Link Creation Agent:
This processor manages link creation requests from the service bus.
)")},
                                                       {ProcessorType::CONTEXT_BROKER, string(R"(
Context Broker:
This processor handles context management requests from the service bus.
Required arguments:
    - attention_broker_address: The address of the Attention Broker to connect to, in the form "host:port"
)")},
                                                       {ProcessorType::EVOLUTION_AGENT, string(R"(
Evolution Agent:
This processor manages query evolution requests from the service bus.
Required arguments:
    - attention_broker_address: The address of the Attention Broker to connect to, in the form "host:port"
)")},
                                                       {ProcessorType::QUERY_ENGINE, string(R"(
Query Engine:
This processor handles pattern matching query requests from the service bus.
Required arguments:
    - attention_broker_address: The address of the Attention Broker to connect to, in the form "host:port"
)")},
                                                       {ProcessorType::UNKNOWN, string(R"(
Usage:
busnode --service=<service> --hostname=<host:port> --ports_range=<start_port:end_port> [--peer_address=<peer_host:peer_port>] --attention_broker_address=<AB_ip:AB_port> [--use_mork=true|false]
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
    - use-cache: Whether to use the cache (true/false)
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
                                                         {ProcessorType::UNKNOWN, string(R"(
Usage:
busclient --service=<service> --hostname=<host:port> --service-hostname=<service_host:service_port> --ports-range=<start_port:end_port> [--use_mork=true|false]
        )")}};

string RunnerHelper::help(const ProcessorType& processor_type, ServiceCallerType caller_type) {
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
        default:
            return string(usage) + string(R"(
Available services:
 - inference-agent
 - link-creation-agent
 - context-broker
 - evolution-agent
 - query-engine
                )");
    }
}

vector<string> RunnerHelper::get_required_arguments(const string& processor_type,
                                                    ServiceCallerType caller_type) {
    ProcessorType p_type = processor_type_from_string(processor_type);
    switch (p_type) {
        case ProcessorType::INFERENCE_AGENT:
            if (caller_type == ServiceCallerType::CLIENT) {
                return {"request", "timeout", "max-answers", "attention-update-flag", "repeat-count"};
            } else {
                return {"attention_broker_address"};
            }
        case ProcessorType::LINK_CREATION_AGENT:
            if (caller_type == ServiceCallerType::CLIENT) {
                return {"request",
                        "max-answers",
                        "repeat-count",
                        "context",
                        "attention-update-flag",
                        "positive-importance-flag",
                        "use-metta-as-query-tokens"};
            } else {
                return {};
            }
        case ProcessorType::CONTEXT_BROKER:
            if (caller_type == ServiceCallerType::CLIENT) {
                return {"context",
                        "query",
                        "determiner-schema",
                        "stimulus-schema",
                        "use-cache",
                        "enforce-cache-recreation"};
            } else {
                return {"attention_broker_address"};
            }
        case ProcessorType::EVOLUTION_AGENT:
            if (caller_type == ServiceCallerType::CLIENT) {
                return {"evolution-request", "max-answers", "attention-update-flag", "repeat-count"};
            } else {
                return {"attention_broker_address"};
            }
        case ProcessorType::QUERY_ENGINE:
            if (caller_type == ServiceCallerType::CLIENT) {
                return {"query", "context"};
            } else {
                return {"attention_broker_address"};
            }
        default:
            return {};
    }
}

ProcessorType RunnerHelper::processor_type_from_string(const string& type_str) {
    if (type_str == "inference-agent") {
        return ProcessorType::INFERENCE_AGENT;
    } else if (type_str == "link-creation-agent") {
        return ProcessorType::LINK_CREATION_AGENT;
    } else if (type_str == "context-broker") {
        return ProcessorType::CONTEXT_BROKER;
    } else if (type_str == "evolution-agent") {
        return ProcessorType::EVOLUTION_AGENT;
    } else if (type_str == "query-engine") {
        return ProcessorType::QUERY_ENGINE;
    } else {
        return ProcessorType::UNKNOWN;
    }
}