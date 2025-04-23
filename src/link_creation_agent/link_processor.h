/**
 * @file link_processor.h
 * @brief Link Processor class
 */

#pragma once
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "PatternMatchingQueryProxy.h"
#include "QueryAnswer.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

using namespace query_engine;
using namespace commons;

namespace link_creation_agent {
class LinkProcessor {
   public:
    LinkProcessor() = default;
    virtual std::vector<std::vector<std::string>> process(
        shared_ptr<QueryAnswer> query_answer,
        std::optional<std::vector<std::string>> extra_params = nullopt) = 0;
    virtual ~LinkProcessor() = default;
    static int count_query(vector<string>& query,
                           mutex& query_mutex,
                           string& context,
                           shared_ptr<service_bus::ServiceBus> service_bus) {
        // lock_guard<mutex> lock(query_mutex);
        shared_ptr<PatternMatchingQueryProxy> query_count_proxy =
            make_shared<PatternMatchingQueryProxy>(query, context, false, true);
        service_bus->issue_bus_command(query_count_proxy);
        while (!query_count_proxy->finished()) {
            Utils::sleep();
        }
        return query_count_proxy->get_count();
    }
};

enum class ProcessorType { PROOF_OF_IMPLICATION, PROOF_OF_EQUIVALENCE, INVALID };

class LinkCreationProcessor {
   public:
    static ProcessorType get_processor_type(string processor) {
        if (processor == "PROOF_OF_IMPLICATION") {
            return ProcessorType::PROOF_OF_IMPLICATION;
        } else if (processor == "PROOF_OF_EQUIVALENCE") {
            return ProcessorType::PROOF_OF_EQUIVALENCE;
        } else {
            return ProcessorType::INVALID;
        }
    }
};
}  // namespace link_creation_agent