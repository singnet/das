/**
 * @file equivalence_processor.h
 */

#pragma once
#include "ServiceBusSingleton.h"
#include "link_processor.h"

using namespace query_engine;
namespace link_creation_agent {
class EquivalenceProcessor : public LinkProcessor {
   public:
    EquivalenceProcessor();
    std::vector<std::vector<std::string>> process(
        shared_ptr<QueryAnswer> query_answer,
        std::optional<std::vector<std::string>> extra_params = nullopt) override;
    void set_das_node(shared_ptr<service_bus::ServiceBus> das_node);
    void set_mutex(shared_ptr<mutex> processor_mutex);

    ~EquivalenceProcessor() = default;

   private:
    static std::vector<std::string> get_pattern_query(const std::string& c1, const std::string& c2);
    shared_ptr<service_bus::ServiceBus> das_node = nullptr;  // check for concurrency
    shared_ptr<mutex> processor_mutex = nullptr;
};
}  // namespace link_creation_agent