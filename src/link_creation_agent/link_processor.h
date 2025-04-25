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

#include "Logger.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryAnswer.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

using namespace query_engine;
using namespace commons;
using namespace std;

namespace link_creation_agent {
class LinkProcessor {
   public:
    LinkProcessor() = default;
    virtual vector<vector<string>> process(shared_ptr<QueryAnswer> query_answer,
                                           optional<vector<string>> extra_params = nullopt) = 0;
    virtual ~LinkProcessor() = default;
    static int count_query(vector<string>& query, string& context, bool is_unique_assignment = true) {
        int count = _count_query(query, context, is_unique_assignment);
        Utils::sleep(500);  // TODO fix this, waiting to delete to not crash
        return count;
    }
    static int _count_query(vector<string>& query, string& context, bool is_unique_assignment = true) {
        try {
            shared_ptr<PatternMatchingQueryProxy> query_count_proxy =
                make_shared<PatternMatchingQueryProxy>(
                    query, context, is_unique_assignment, false, true);
            ServiceBusSingleton::get_instance()->issue_bus_command(query_count_proxy);
            while (!query_count_proxy->finished()) {
                Utils::sleep(20);
            }
            return query_count_proxy->get_count();
        } catch (const exception& e) {
            LOG_ERROR("Exception: " << e.what());
            return 0;
        }
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