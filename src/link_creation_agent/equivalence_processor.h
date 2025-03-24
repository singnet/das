/**
 * @file equivalence_processor.h
 */

 #pragma once
#include "link_processor.h"

namespace link_creation_agent {
    class EquivalenceProcessor : public LinkProcessor {
        public:
            EquivalenceProcessor() = default;
            EquivalenceProcessor(QueryAnswer* query_answer) : LinkProcessor(query_answer) {};
            std::vector<std::vector<std::string>> get_links() override;
            void process() override;
            ~EquivalenceProcessor() = default;
    };
}  // namespace link_creation_agent