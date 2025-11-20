/**
 * @file CommandProcessorFactory.h
 */

#pragma once

#include "BusCommandProcessor.h"
#include "ContextBrokerProcessor.h"
#include "InferenceProcessor.h"
#include "LinkCreationRequestProcessor.h"
#include "PatternMatchingQueryProcessor.h"
#include "Properties.h"
#include "QueryEvolutionProcessor.h"
#include "RunnerHelper.h"
#include "Utils.h"

using namespace std;
using namespace service_bus;
using namespace inference_agent;
using namespace link_creation_agent;
using namespace context_broker;
using namespace query_engine;
using namespace evolution;
using namespace atomdb;

namespace mains {

class ProcessorFactory {
   public:
    ProcessorFactory();
    ~ProcessorFactory();
    /**
     * Creates a BusCommandProcessor based on the provided type and parameters.
     * @param processor_type The type of the processor to create, types are: "inference-agent",
     * "link-creation-agent", "evolution-agent", "query-engine", "context-broker".
     * @param params A map of parameters required for the processor creation.
     * @return A shared pointer to the created BusCommandProcessor.
     */
    static shared_ptr<BusCommandProcessor> create_processor(const string& processor_type,
                                                            const Properties& params) {
        ProcessorType p_type = RunnerHelper::processor_type_from_string(processor_type);
        if (p_type == ProcessorType::INFERENCE_AGENT) {
            return make_shared<InferenceProcessor>();

        } else if (p_type == ProcessorType::LINK_CREATION_AGENT) {
            return make_shared<LinkCreationRequestProcessor>(params);

        } else if (p_type == ProcessorType::CONTEXT_BROKER) {
            return make_shared<ContextBrokerProcessor>();

        } else if (p_type == ProcessorType::EVOLUTION_AGENT) {
            return make_shared<QueryEvolutionProcessor>();

        } else if (p_type == ProcessorType::QUERY_ENGINE) {
            return make_shared<PatternMatchingQueryProcessor>();

        } else {
            Utils::error("Unknown processor type: " + processor_type);
        }
    };
};

}  // namespace mains