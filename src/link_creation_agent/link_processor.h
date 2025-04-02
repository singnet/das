/**
 * @file link_processor.h
 * @brief Link Processor class
 */

#pragma once
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>

#include "QueryAnswer.h"

using namespace query_engine;

namespace link_creation_agent {
class LinkProcessor {
   public:
    LinkProcessor() = default;
    virtual std::vector<std::vector<std::string>> process(
        QueryAnswer* query_answer, std::optional<std::vector<std::string>> extra_params = nullopt) = 0;
    virtual ~LinkProcessor() = default;
};

enum class ProcessorType {
    PROOF_OF_IMPLICATION,
    PROOF_OF_EQUIVALENCE,
    INVALID
};

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