/**
 * @file implication_processor.h
 */

#pragma once
#include "link_processor.h"

namespace link_creation_agent {
    class ImplicationProcessor : public LinkProcessor {
        public:
            ImplicationProcessor() = default;
            ImplicationProcessor(QueryAnswer* query_answer) : LinkProcessor(query_answer) {};
            std::vector<std::vector<std::string>> get_links() override;
            void process() override;
            ~ImplicationProcessor() = default;
    };
}  // namespace link_creation_agent