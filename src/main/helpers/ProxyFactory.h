#pragma once

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

namespace mains {
class ProxyFactory {
   public:
    ProxyFactory() = default;
    ~ProxyFactory() = default;
    static shared_ptr<BaseProxy> create_proxy(const string& proxy_type, const Properties& params) {
        auto processor_type = RunnerHelper::processor_type_from_string(proxy_type);
        if (processor_type == ProcessorType::INFERENCE_AGENT) {
            auto request = params.get_or<string>("request", "");
            auto timeout = params.get_or<string>("timeout", "");
            auto max_answers = params.get_or<string>("max-answers", "");
            auto attention_update_flag = params.get_or<string>("attention-update-flag", "");
            auto repeat_count = params.get_or<string>("repeat-count", "");
            auto proxy = make_shared<InferenceProxy>(Utils::split(request, ' '));
            if (!timeout.empty()) {
                proxy->parameters[InferenceProxy::INFERENCE_REQUEST_TIMEOUT] =
                    (unsigned int) stoi(timeout);
            }
            if (!max_answers.empty()) {
                proxy->parameters[InferenceProxy::MAX_ANSWERS] = (unsigned int) stoi(max_answers);
            }
            if (!attention_update_flag.empty()) {
                proxy->parameters[InferenceProxy::ATTENTION_UPDATE_FLAG] =
                    (attention_update_flag == "true" || attention_update_flag == "1");
            }
            if (!repeat_count.empty()) {
                proxy->parameters[InferenceProxy::REPEAT_COUNT] = (unsigned int) stoi(repeat_count);
            }
            return proxy;

        } else if (processor_type == ProcessorType::LINK_CREATION_AGENT) {
            auto request = params.get_or<string>("request", "");
            auto max_answers = params.get_or<string>("max-answers", "");
            auto repeat_count = params.get_or<string>("repeat-count", "");
            auto context = params.get_or<string>("context", "");
            auto attention_update_flag = params.get_or<string>("attention-update-flag", "");
            auto positive_importance_flag = params.get_or<string>("positive-importance-flag", "");
            auto use_metta_as_query_tokens = params.get_or<string>("use-metta-as-query-tokens", "");
            auto proxy = make_shared<LinkCreationRequestProxy>(Utils::split(request, ' '));
            if (!max_answers.empty()) {
                proxy->parameters[LinkCreationRequestProxy::MAX_ANSWERS] =
                    (unsigned int) stoi(max_answers);
            }
            if (!repeat_count.empty()) {
                proxy->parameters[LinkCreationRequestProxy::REPEAT_COUNT] =
                    (unsigned int) stoi(repeat_count);
            }
            if (!context.empty()) {
                proxy->parameters[LinkCreationRequestProxy::CONTEXT] = context;
            }
            if (!attention_update_flag.empty()) {
                proxy->parameters[LinkCreationRequestProxy::ATTENTION_UPDATE_FLAG] =
                    (attention_update_flag == "true" || attention_update_flag == "1");
            }
            if (!positive_importance_flag.empty()) {
                proxy->parameters[LinkCreationRequestProxy::POSITIVE_IMPORTANCE_FLAG] =
                    (positive_importance_flag == "true" || positive_importance_flag == "1");
            }
            if (!use_metta_as_query_tokens.empty()) {
                proxy->parameters[LinkCreationRequestProxy::USE_METTA_AS_QUERY_TOKENS] =
                    (use_metta_as_query_tokens == "true" || use_metta_as_query_tokens == "1");
            }
            return proxy;

        } else if (processor_type == ProcessorType::CONTEXT_BROKER) {
            auto context = params.get<string>("context");
            auto query = params.get<string>("query");
            auto determiner_schema = params.get<string>("determiner-schema");
            auto stimulus_schema = params.get<string>("stimulus-schema");
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

        } else if (processor_type == ProcessorType::EVOLUTION_AGENT) {
            auto query = params.get<string>("query");
            auto correlation_queries = params.get<string>("correlation-queries");
            auto correlation_replacements = params.get<string>("correlation-replacements");
            auto correlation_mappings = params.get<string>("correlation-mappings");
            auto context = params.get<string>("context");
            auto fitness_function_tag = params.get<string>("fitness-function-tag");
            // auto proxy = make_shared<QueryEvolutionProxy>();
            return nullptr;

        } else if (processor_type == ProcessorType::QUERY_ENGINE) {
            auto query = params.get<string>("query");
            auto context = params.get<string>("context");
            auto proxy = make_shared<PatternMatchingQueryProxy>(Utils::split(query, ' '), context);
            auto unique_assignment_flag = params.get_or<string>("unique-assignment-flag", "");
            auto attention_update_flag = params.get_or<string>("attention-update-flag", "");
            auto max_answers = params.get_or<string>("max-answers", "");
            auto use_link_template_cache = params.get_or<string>("use-link-template-cache", "");
            auto populate_metta_mapping = params.get_or<string>("populate-metta-mapping", "");
            auto use_metta_as_query_tokens = params.get_or<string>("use-metta-as-query-tokens", "");
            auto positive_importance_flag = params.get_or<string>("positive-importance-flag", "");
            if (!unique_assignment_flag.empty()) {
                proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] =
                    (unique_assignment_flag == "true" || unique_assignment_flag == "1");
            }
            if (!attention_update_flag.empty()) {
                proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] =
                    (attention_update_flag == "true" || attention_update_flag == "1");
            }
            if (!max_answers.empty()) {
                proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] =
                    (unsigned int) stoi(max_answers);
            }
            if (!use_link_template_cache.empty()) {
                proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] =
                    (use_link_template_cache == "true" || use_link_template_cache == "1");
            }
            if (!populate_metta_mapping.empty()) {
                proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] =
                    (populate_metta_mapping == "true" || populate_metta_mapping == "1");
            }
            if (!use_metta_as_query_tokens.empty()) {
                proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] =
                    (use_metta_as_query_tokens == "true" || use_metta_as_query_tokens == "1");
            }
            if (!positive_importance_flag.empty()) {
                proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] =
                    (positive_importance_flag == "true" || positive_importance_flag == "1");
            }
            return proxy;
        } else {
            Utils::error("Unknown proxy type: " + proxy_type);
        }
    }
};
}  // namespace mains