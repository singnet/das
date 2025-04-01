/**
 * @file implication_processor.h
 */

#pragma once
#include "link_processor.h"

namespace link_creation_agent {
class ImplicationProcessor : public LinkProcessor {
   public:
    ImplicationProcessor() = default;
    std::vector<std::vector<std::string>> process(
        QueryAnswer* query_answer,
        std::optional<std::vector<std::string>> extra_params = nullopt) override;
};
}  // namespace link_creation_agent