#pragma once

#include "AtomDBProxy.h"
#include "BaseProxy.h"
#include "ContextBrokerProxy.h"
#include "Helper.h"
#include "InferenceProxy.h"
#include "LinkCreationRequestProxy.h"
#include "PatternMatchingQueryProxy.h"
#include "Properties.h"
#include "QueryEvolutionProxy.h"
#include "Utils.h"

using namespace std;
using namespace commons;
using namespace agents;
using namespace inference_agent;
using namespace link_creation_agent;
using namespace context_broker;
using namespace evolution;
using namespace query_engine;
using namespace atomdb_broker;

namespace mains {
class ProxyFactory {
   public:
    ProxyFactory() = default;
    ~ProxyFactory() = default;

    enum class ParamType { STRING, LONG, UINT, DOUBLE, BOOL };

    static void set_param(shared_ptr<BaseProxy> proxy,
                          const string& param_name,
                          const string& param_value,
                          const ParamType param_type) {
        if (param_value.empty()) {
            return;
        }
        switch (param_type) {
            case ParamType::UINT:
                proxy->parameters[param_name] = (unsigned int) stoi(param_value);
                return;
            case ParamType::DOUBLE:
                proxy->parameters[param_name] = (double) stod(param_value);
                return;
            case ParamType::LONG:
                proxy->parameters[param_name] = (long) stol(param_value);
                return;
            case ParamType::BOOL:
                proxy->parameters[param_name] = (param_value == "true" || param_value == "1");
                return;
            case ParamType::STRING:
                proxy->parameters[param_name] = param_value;
                return;
            default:
                Utils::error("Unknown proxy parameter type");
        }
    }

    static string parse_metta_expression(const string& metta_expression) {
        string parsed_expression = metta_expression;
        Utils::replace_all(parsed_expression, "%", "$");
        return parsed_expression;
    }

    static vector<string> parse_request(const string& request_str,
                                        const string& use_metta_as_query_tokens,
                                        bool single_metta = false) {
        vector<string> request_tokens;
        if (use_metta_as_query_tokens == "true" || use_metta_as_query_tokens == "1") {
            string parsed_request = parse_metta_expression(request_str);
            if (single_metta) {
                request_tokens.push_back(parsed_request);

            } else {
                request_tokens = Utils::split(parsed_request, ' ');
            }
        } else {
            request_tokens = Utils::split(request_str, ' ');
        }
        return request_tokens;
    }

    static vector<vector<string>> parse_correlation_queries(const string& correlation_queries_str,
                                                            const string& use_metta_as_query_tokens) {
        string parsed_correlation_queries_str = correlation_queries_str;
        if (use_metta_as_query_tokens == "true") {
            parsed_correlation_queries_str = parse_metta_expression(correlation_queries_str);
        }
        vector<string> correlation_queries_vec = Utils::split(parsed_correlation_queries_str, ',');
        vector<vector<string>> correlation_queries;
        for (auto& q : correlation_queries_vec) {
            correlation_queries.push_back(parse_request(q, use_metta_as_query_tokens, true));
        }
        return correlation_queries;
    }

    static vector<map<string, QueryAnswerElement>> parse_correlation_replacements(
        const string& correlation_replacements_str, const string& use_metta_as_query_tokens) {
        string parsed_correlation_replacements_str = correlation_replacements_str;
        if (use_metta_as_query_tokens == "true") {
            parsed_correlation_replacements_str = parse_metta_expression(correlation_replacements_str);
        }
        auto constants_list = Utils::split(parsed_correlation_replacements_str, ';');
        vector<map<string, QueryAnswerElement>> correlation_replacements;
        for (auto value : constants_list) {
            auto mapping = Utils::split(value, ',');
            map<string, QueryAnswerElement> replacement_map;
            for (auto pair_str : mapping) {
                auto pair = Utils::split(pair_str, ':');
                if (pair.size() == 2) {
                    replacement_map[pair[0]] = QueryAnswerElement(pair[1]);
                }
            }
            correlation_replacements.push_back(replacement_map);
        }
        return correlation_replacements;
    }

    static vector<pair<QueryAnswerElement, QueryAnswerElement>> parse_correlation_mappings(
        const string& correlation_mappings_str, const string& use_metta_as_query_tokens) {
        string parsed_correlation_mappings_str = correlation_mappings_str;
        if (use_metta_as_query_tokens == "true") {
            parsed_correlation_mappings_str = parse_metta_expression(correlation_mappings_str);
        }
        vector<pair<QueryAnswerElement, QueryAnswerElement>> correlation_mappings;
        auto mapping_list = Utils::split(parsed_correlation_mappings_str, ',');
        for (auto value : mapping_list) {
            auto pair = Utils::split(value, ':');
            if (pair.size() == 2) {
                correlation_mappings.push_back(
                    make_pair(QueryAnswerElement(pair[0]), QueryAnswerElement(pair[1])));
            }
        }
        return correlation_mappings;
    }

    static void set_base_query_params(shared_ptr<BaseProxy> proxy, const Properties& params) {
        string context = params.get<string>(Helper::CONTEXT);
        string uaf = params.get_or<string>(Helper::UNIQUE_ASSIGNMENT_FLAG, "");
        string auf = params.get_or<string>(Helper::ATTENTION_UPDATE_FLAG, "");
        string max_answers = params.get_or<string>(Helper::MAX_ANSWERS, "");
        string ultc = params.get_or<string>(Helper::USE_LINK_TEMPLATE_CACHE, "");
        string pmm = params.get_or<string>(Helper::POPULATE_METTA_MAPPING, "");
        string umaqt = params.get_or<string>(Helper::USE_METTA_AS_QUERY_TOKENS, "");
        set_param(proxy, BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG, uaf, ParamType::BOOL);
        set_param(proxy, BaseQueryProxy::ATTENTION_UPDATE_FLAG, auf, ParamType::BOOL);
        set_param(proxy, BaseQueryProxy::MAX_ANSWERS, max_answers, ParamType::UINT);
        set_param(proxy, BaseQueryProxy::USE_LINK_TEMPLATE_CACHE, ultc, ParamType::BOOL);
        set_param(proxy, BaseQueryProxy::POPULATE_METTA_MAPPING, pmm, ParamType::BOOL);
        set_param(proxy, BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS, umaqt, ParamType::BOOL);
    }

    static shared_ptr<BaseProxy> create_proxy(const string& proxy_type, const Properties& params) {
        auto processor_type = Helper::processor_type_from_string(proxy_type);
        string context = params.get<string>(Helper::CONTEXT);
        string pif = params.get_or<string>(Helper::POSITIVE_IMPORTANCE_FLAG, "");
        string umaqt = params.get_or<string>(Helper::USE_METTA_AS_QUERY_TOKENS, "");

        switch (processor_type) {
            case ProcessorType::INFERENCE_AGENT: {
                string request = params.get_or<string>(Helper::REQUEST, "");
                string timeout = params.get_or<string>(Helper::TIMEOUT, "");
                string repeat_count = params.get_or<string>(Helper::REPEAT_COUNT, "");
                auto proxy = make_shared<InferenceProxy>(Utils::split(request, ' '));
                set_param(proxy, InferenceProxy::INFERENCE_REQUEST_TIMEOUT, timeout, ParamType::UINT);
                set_base_query_params(proxy, params);
                set_param(proxy, InferenceProxy::REPEAT_COUNT, repeat_count, ParamType::UINT);
                return proxy;
            }
            case ProcessorType::LINK_CREATION_AGENT: {
                string request_str = params.get_or<string>(Helper::REQUEST, "");
                string repeat_count = params.get_or<string>(Helper::REPEAT_COUNT, "");
                auto request = parse_request(request_str, umaqt);
                auto proxy = make_shared<LinkCreationRequestProxy>(request);
                set_param(
                    proxy, LinkCreationRequestProxy::POSITIVE_IMPORTANCE_FLAG, pif, ParamType::BOOL);
                set_param(proxy, LinkCreationRequestProxy::REPEAT_COUNT, repeat_count, ParamType::UINT);
                set_base_query_params(proxy, params);
                return proxy;
            }
            case ProcessorType::CONTEXT_BROKER: {
                string query_str = params.get_or<string>("query", "");
                auto query = parse_request(query_str, umaqt, true);
                string determiner_schema = params.get<string>(Helper::DETERMINER_SCHEMA);
                string stimulus_schema = params.get<string>(Helper::STIMULUS_SCHEMA);
                auto proxy =
                    make_shared<ContextBrokerProxy>(context, query, determiner_schema, stimulus_schema);
                set_param(proxy,
                          ContextBrokerProxy::USE_CACHE,
                          params.get_or<string>(Helper::USE_CONTEXT_CACHE, ""),
                          ParamType::BOOL);
                set_param(proxy,
                          ContextBrokerProxy::ENFORCE_CACHE_RECREATION,
                          params.get_or<string>(Helper::ENFORCE_CACHE_RECREATION, ""),
                          ParamType::BOOL);
                set_param(proxy,
                          ContextBrokerProxy::INITIAL_RENT_RATE,
                          params.get_or<string>(Helper::INITIAL_RENT_RATE, ""),
                          ParamType::DOUBLE);
                set_param(proxy,
                          ContextBrokerProxy::INITIAL_SPREADING_RATE_LOWERBOUND,
                          params.get_or<string>(Helper::INITIAL_SPREADING_RATE_LOWERBOUND, ""),
                          ParamType::DOUBLE);
                set_param(proxy,
                          ContextBrokerProxy::INITIAL_SPREADING_RATE_UPPERBOUND,
                          params.get_or<string>(Helper::INITIAL_SPREADING_RATE_UPPERBOUND, ""),
                          ParamType::DOUBLE);
                set_base_query_params(proxy, params);
                return proxy;
            }
            case ProcessorType::EVOLUTION_AGENT: {
                string query_str = params.get<string>("query");
                auto query = parse_request(query_str, umaqt, true);
                string correlation_queries_str = params.get<string>(Helper::CORRELATION_QUERIES);
                auto correlation_queries = parse_correlation_queries(correlation_queries_str, umaqt);
                string correlation_replacements_str =
                    params.get<string>(Helper::CORRELATION_REPLACEMENTS);
                auto correlation_replacements =
                    parse_correlation_replacements(correlation_replacements_str, umaqt);
                string correlation_mappings_str = params.get<string>(Helper::CORRELATION_MAPPINGS);
                auto correlation_mappings = parse_correlation_mappings(correlation_mappings_str, umaqt);
                string fitness_function_tag = params.get<string>(Helper::FITNESS_FUNCTION_TAG);
                string population_size = params.get_or<string>(Helper::POPULATION_SIZE, "");
                string max_generations = params.get_or<string>(Helper::MAX_GENERATIONS, "");
                string elitism_rate = params.get_or<string>(Helper::ELITISM_RATE, "");
                string selection_rate = params.get_or<string>(Helper::SELECTION_RATE, "");
                string total_attention_tokens =
                    params.get_or<string>(Helper::TOTAL_ATTENTION_TOKENS, "");
                auto proxy = make_shared<QueryEvolutionProxy>(query,
                                                              correlation_queries,
                                                              correlation_replacements,
                                                              correlation_mappings,
                                                              context,
                                                              fitness_function_tag);
                set_base_query_params(proxy, params);
                set_param(proxy, QueryEvolutionProxy::POPULATION_SIZE, population_size, ParamType::UINT);
                set_param(proxy, QueryEvolutionProxy::MAX_GENERATIONS, max_generations, ParamType::UINT);
                set_param(proxy, QueryEvolutionProxy::ELITISM_RATE, elitism_rate, ParamType::DOUBLE);
                set_param(proxy, QueryEvolutionProxy::SELECTION_RATE, selection_rate, ParamType::DOUBLE);
                set_param(proxy,
                          QueryEvolutionProxy::TOTAL_ATTENTION_TOKENS,
                          total_attention_tokens,
                          ParamType::UINT);
                return proxy;
            }
            case ProcessorType::QUERY_ENGINE: {
                string query_str = params.get<string>("query");
                auto query = parse_request(query_str, umaqt, true);
                auto proxy = make_shared<PatternMatchingQueryProxy>(query, context);
                set_base_query_params(proxy, params);
                set_param(
                    proxy, PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG, pif, ParamType::BOOL);
                return proxy;
            }
            case ProcessorType::ATOMDB_BROKER: {
                return make_shared<AtomDBProxy>();
            }
            default:
                Utils::error("Unknown proxy type: " + proxy_type);

                break;
        }
    }
};
}  // namespace mains