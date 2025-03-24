/**
 * @file link_processor.h
 * @brief Link Processor class
 */

#pragma once
#include "QueryAnswer.h"
#include <vector>
#include <string>

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
}  // namespace link_creation_agent