/**
 * @file implication_processor.h
 */

#pragma once
#include "link_processor.h"
#include "DASNode.h"

using namespace query_engine;

namespace link_creation_agent {
class ImplicationProcessor : public LinkProcessor {
   public:
    ImplicationProcessor();
    std::vector<std::vector<std::string>> process(
        QueryAnswer* query_answer,
        std::optional<std::vector<std::string>> extra_params = nullopt) override;
        void set_das_node(shared_ptr<DASNode> das_node);
        void set_mutex(shared_ptr<mutex> processor_mutex);
    private:
        static std::vector<std::string> get_satisfying_set_query(const std::string& p1, const std::string& p2);
        shared_ptr<DASNode> das_node = nullptr; // check for concurrency
        shared_ptr<mutex> processor_mutex = nullptr;
};
}  // namespace link_creation_agent