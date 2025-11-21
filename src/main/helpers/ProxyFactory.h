#pragma once

#include "AtomDBProxy.h"
#include "BaseProxy.h"
#include "ContextBrokerProxy.h"
#include "InferenceProxy.h"
#include "LinkCreationRequestProxy.h"
#include "PatternMatchingQueryProxy.h"
#include "Properties.h"
#include "QueryEvolutionProxy.h"
#include "RunnerHelper.h"
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

    static void set_param(Properties& proxy_params,
                          const string& param_name,
                          const string& param_value,
                          const ParamType param_type) {
        if (param_value.empty()) {
            return;
        }
        LOG_INFO("Setting proxy parameter: " << param_name << " to value: " << param_value);
        switch (param_type) {
            case ParamType::UINT:
                proxy_params[param_name] = (unsigned int) stoi(param_value);
                return;
            case ParamType::DOUBLE:
                proxy_params[param_name] = (double) stod(param_value);
                return;
            case ParamType::LONG:
                proxy_params[param_name] = (long) stol(param_value);
                return;
            case ParamType::BOOL:
                proxy_params[param_name] = (param_value == "true" || param_value == "1");
                return;
            case ParamType::STRING:
                proxy_params[param_name] = param_value;
                return;
            default:
                Utils::error("Unknown proxy parameter type");
        }
    }

    static shared_ptr<BaseProxy> create_proxy(const string& proxy_type, const Properties& params) {
        auto processor_type = RunnerHelper::processor_type_from_string(proxy_type);
        switch (processor_type) {
            case ProcessorType::INFERENCE_AGENT: {
                string request = params.get_or<string>(RunnerHelper::REQUEST, "");
                string timeout = params.get_or<string>(RunnerHelper::TIMEOUT, "");
                string max_answers = params.get_or<string>(RunnerHelper::MAX_ANSWERS, "");
                string auf = params.get_or<string>(RunnerHelper::ATTENTION_UPDATE_FLAG, "");
                string repeat_count = params.get_or<string>(RunnerHelper::REPEAT_COUNT, "");
                auto proxy = make_shared<InferenceProxy>(Utils::split(request, ' '));
                auto params = &proxy->parameters;
                set_param(*params, InferenceProxy::INFERENCE_REQUEST_TIMEOUT, timeout, ParamType::UINT);
                set_param(*params, InferenceProxy::MAX_ANSWERS, max_answers, ParamType::UINT);
                set_param(*params, InferenceProxy::ATTENTION_UPDATE_FLAG, auf, ParamType::BOOL);
                set_param(*params, InferenceProxy::REPEAT_COUNT, repeat_count, ParamType::UINT);
                return proxy;
            }
            case ProcessorType::LINK_CREATION_AGENT: {
                string request = params.get_or<string>(RunnerHelper::REQUEST, "");
                string max_answers = params.get_or<string>(RunnerHelper::MAX_ANSWERS, "");
                string repeat_count = params.get_or<string>(RunnerHelper::REPEAT_COUNT, "");
                string context = params.get_or<string>(RunnerHelper::CONTEXT, "");
                string auf = params.get_or<string>(RunnerHelper::ATTENTION_UPDATE_FLAG, "");
                string pif = params.get_or<string>(RunnerHelper::POSITIVE_IMPORTANCE_FLAG, "");
                string umaqt = params.get_or<string>(RunnerHelper::USE_METTA_AS_QUERY_TOKENS, "");
                if (umaqt == "true" || umaqt == "1") {
                    Utils::replace_all(request, "%", "$");
                }
                auto proxy = make_shared<LinkCreationRequestProxy>(Utils::split(request, ' '));
                auto params = &proxy->parameters;
                set_param(*params, LinkCreationRequestProxy::MAX_ANSWERS, max_answers, ParamType::UINT);
                set_param(
                    *params, LinkCreationRequestProxy::REPEAT_COUNT, repeat_count, ParamType::UINT);
                set_param(*params, LinkCreationRequestProxy::CONTEXT, context, ParamType::STRING);
                set_param(
                    *params, LinkCreationRequestProxy::ATTENTION_UPDATE_FLAG, auf, ParamType::BOOL);
                set_param(
                    *params, LinkCreationRequestProxy::POSITIVE_IMPORTANCE_FLAG, pif, ParamType::BOOL);
                set_param(*params,
                          LinkCreationRequestProxy::USE_METTA_AS_QUERY_TOKENS,
                          umaqt,
                          ParamType::BOOL);
                return proxy;
            }
            case ProcessorType::CONTEXT_BROKER: {
                string context = params.get<string>(RunnerHelper::CONTEXT);
                string query = params.get_or<string>(
                    "query", "");  // Note to reviewer: RunnerHelper::QUERY is not linking
                string determiner_schema = params.get<string>(RunnerHelper::DETERMINER_SCHEMA);
                string stimulus_schema = params.get<string>(RunnerHelper::STIMULUS_SCHEMA);
                // auto proxy = make_shared<ContextBrokerProxy>();
                // auto use_cache = params.get<string>("use-cache");
                // auto enforce_cache_recreation = params.get<string>("enforce-cache-recreation");
                // auto initial_rent_rate = params.get<string>("initial-rent-rate");
                // auto initial_spreading_rate_lowerbound =
                //     params.get<string>("initial-spreading-rate-lowerbound");
                // auto initial_spreading_rate_upperbound =
                //     params.get<string>("initial-spreading-rate-upperbound");
                // proxy->parameters[ContextBrokerProxy::USE_CACHE] =
                //     (use_cache == "true" || use_cache == "1");
                // proxy->parameters[ContextBrokerProxy::ENFORCE_CACHE_RECREATION] =
                //     (enforce_cache_recreation == "true" || enforce_cache_recreation == "1");
                // proxy->parameters[ContextBrokerProxy::INITIAL_RENT_RATE] = stod(initial_rent_rate);
                // proxy->parameters[ContextBrokerProxy::INITIAL_SPREADING_RATE_LOWERBOUND] =
                //     stod(initial_spreading_rate_lowerbound);
                // proxy->parameters[ContextBrokerProxy::INITIAL_SPREADING_RATE_UPPERBOUND] =
                //     stod(initial_spreading_rate_upperbound);
                return nullptr;
            }
            case ProcessorType::EVOLUTION_AGENT: {
                string query =
                    params.get<string>("query");  // Note to reviewer: RunnerHelper::QUERY is not linking
                string correlation_queries = params.get<string>(RunnerHelper::CORRELATION_QUERIES);
                string correlation_replacements =
                    params.get<string>(RunnerHelper::CORRELATION_REPLACEMENTS);
                string correlation_mappings = params.get<string>(RunnerHelper::CORRELATION_MAPPINGS);
                string context = params.get<string>(RunnerHelper::CONTEXT);
                string fitness_function_tag = params.get<string>(RunnerHelper::FITNESS_FUNCTION_TAG);
                // auto proxy = make_shared<QueryEvolutionProxy>();
                return nullptr;
            }
            case ProcessorType::QUERY_ENGINE: {
                string query =
                    params.get<string>("query");  // Note to reviewer: RunnerHelper::QUERY is not linking
                string context = params.get<string>(RunnerHelper::CONTEXT);
                auto proxy = make_shared<PatternMatchingQueryProxy>(Utils::split(query, ' '), context);
                string uaf = params.get_or<string>(RunnerHelper::UNIQUE_ASSIGNMENT_FLAG, "");
                string auf = params.get_or<string>(RunnerHelper::ATTENTION_UPDATE_FLAG, "");
                string max_answers = params.get_or<string>(RunnerHelper::MAX_ANSWERS, "");
                string ultc = params.get_or<string>(RunnerHelper::USE_LINK_TEMPLATE_CACHE, "");
                string pmm = params.get_or<string>(RunnerHelper::POPULATE_METTA_MAPPING, "");
                string umaqt = params.get_or<string>(RunnerHelper::USE_METTA_AS_QUERY_TOKENS, "");
                string pif = params.get_or<string>(RunnerHelper::POSITIVE_IMPORTANCE_FLAG, "");
                auto params = &proxy->parameters;
                set_param(*params, BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG, uaf, ParamType::BOOL);
                set_param(*params, BaseQueryProxy::ATTENTION_UPDATE_FLAG, auf, ParamType::BOOL);
                set_param(*params, PatternMatchingQueryProxy::MAX_ANSWERS, max_answers, ParamType::UINT);
                set_param(*params, BaseQueryProxy::USE_LINK_TEMPLATE_CACHE, ultc, ParamType::BOOL);
                set_param(*params, BaseQueryProxy::POPULATE_METTA_MAPPING, pmm, ParamType::BOOL);
                set_param(*params, BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS, umaqt, ParamType::BOOL);
                set_param(
                    *params, PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG, pif, ParamType::BOOL);
                return proxy;
            }
            case ProcessorType::ATOMDB_BROKER: {
                // string action = params.get_or<string>("action", "");
                // string tokens = params.get_or<string>("tokens", "");
                return make_shared<AtomDBProxy>();
            }
            default:
                Utils::error("Unknown proxy type: " + proxy_type);

                break;
        }
    }
};
}  // namespace mains