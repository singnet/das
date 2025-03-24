/**
 * @file link_processor.h
 * @brief Link Processor class
 */

#pragma once
#include <string>
#include <vector>
#include <stdexcept>


#include "QueryAnswer.h"

using namespace query_engine;

namespace link_creation_agent {
class LinkProcessor {
   public:
    LinkProcessor() = default;
    LinkProcessor(QueryAnswer* query_answer) : query_answer(query_answer) {};
    virtual std::vector<std::vector<std::string>> get_links() = 0;
    virtual void process() = 0;
    virtual ~LinkProcessor() = default;

   protected:
    QueryAnswer* query_answer;
};

enum class ProcessorType {
    PROOF_OF_IMPLICATION,
    PROOF_OF_EQUIVALENCE,
};

class LinkCreationProcessor {
   public:
    static ProcessorType get_processor_type(string processor) {
        if (processor == "PROOF_OF_IMPLICATION") {
            return ProcessorType::PROOF_OF_IMPLICATION;
        } else if (processor == "PROOF_OF_EQUIVALENCE") {
            return ProcessorType::PROOF_OF_EQUIVALENCE;
        } else {
            throw runtime_error("Invalid processor type");
        }
    }
};
}  // namespace link_creation_agent