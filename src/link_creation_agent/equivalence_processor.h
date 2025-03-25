/**
 * @file equivalence_processor.h
 */

#pragma once
#include "link_processor.h"

namespace link_creation_agent {
class EquivalenceProcessor : public LinkProcessor {
   public:
    EquivalenceProcessor() = default;
    std::vector<std::vector<std::string>> process(
        QueryAnswer* query_answer,
        std::optional<std::vector<std::string>> config = std::nullopt) override;
    ~EquivalenceProcessor() = default;
};
}  // namespace link_creation_agent