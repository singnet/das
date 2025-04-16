/**
 * @file equivalence_processor.h
 */

#pragma once
#include "link_processor.h"
#include "DASNode.h"
using namespace query_engine;
namespace link_creation_agent {
class EquivalenceProcessor : public LinkProcessor {
   public:
    EquivalenceProcessor();
    std::vector<std::vector<std::string>> process(
        QueryAnswer* query_answer,
        std::optional<std::vector<std::string>> extra_params = nullopt) override;
        void set_das_node(shared_ptr<DASNode> das_node);
        void set_mutex(shared_ptr<mutex> processor_mutex);

    ~EquivalenceProcessor() = default;
    private:
        static std::vector<std::string> get_pattern_query(const std::string& c1, const std::string& c2);
        shared_ptr<DASNode> das_node = nullptr; // check for concurrency
        shared_ptr<mutex> processor_mutex = nullptr;

};
}  // namespace link_creation_agent