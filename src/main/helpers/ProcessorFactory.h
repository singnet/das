/**
 * @file CommandProcessorFactory.h
 */

#pragma once

#include "AtomDBProcessor.h"
#include "BusCommandProcessor.h"
#include "ContextBrokerProcessor.h"
#include "InferenceProcessor.h"
#include "LinkCreationRequestProcessor.h"
#include "PatternMatchingQueryProcessor.h"
#include "Properties.h"
#include "QueryEvolutionProcessor.h"
#include "Helper.h"
#include "Utils.h"

using namespace std;
using namespace service_bus;
using namespace inference_agent;
using namespace link_creation_agent;
using namespace context_broker;
using namespace query_engine;
using namespace evolution;
using namespace atomdb;
using namespace atomdb_broker;

namespace mains {

class ProcessorFactory {
   public:
    ProcessorFactory();
    ~ProcessorFactory();
    static shared_ptr<BusCommandProcessor> create_processor(const string& processor_type,
                                                            const Properties& params) {
        ProcessorType p_type = Helper::processor_type_from_string(processor_type);
        switch (p_type) {
            case ProcessorType::ATOMDB_BROKER:
                return make_shared<AtomDBProcessor>();
            case ProcessorType::INFERENCE_AGENT:
                return make_shared<InferenceProcessor>();
            case ProcessorType::LINK_CREATION_AGENT:
                return make_shared<LinkCreationRequestProcessor>(params);
            case ProcessorType::CONTEXT_BROKER:
                return make_shared<ContextBrokerProcessor>();
            case ProcessorType::EVOLUTION_AGENT:
                return make_shared<QueryEvolutionProcessor>();
            case ProcessorType::QUERY_ENGINE:
                return make_shared<PatternMatchingQueryProcessor>();
            default:
                Utils::error("Unknown processor type: " + processor_type);
        }
    };
};

}  // namespace mains