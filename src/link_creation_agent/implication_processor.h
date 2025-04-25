/**
 * @file implication_processor.h
 */

#pragma once
#include "ServiceBusSingleton.h"
#include "link_processor.h"

using namespace query_engine;

namespace link_creation_agent {
class ImplicationProcessor : public LinkProcessor {
   public:
    ImplicationProcessor();
    std::vector<std::vector<std::string>> process(
        shared_ptr<QueryAnswer> query_answer,
        std::optional<std::vector<std::string>> extra_params = nullopt) override;
    void set_das_node(shared_ptr<service_bus::ServiceBus> das_node);
    void set_mutex(shared_ptr<mutex> processor_mutex);

   private:
    // static std::vector<std::string> get_satisfying_set_query(const std::string& p1,
    //                                                          const std::string& p2);
    static std::vector<std::string> get_pattern_query(const std::vector<std::string>& p);
    static std::vector<std::string> get_satisfying_set_query(const std::vector<std::string>& p1,
                                                             const std::vector<std::string>& p2);
    shared_ptr<service_bus::ServiceBus> das_node = nullptr;  // check for concurrency
    shared_ptr<mutex> processor_mutex = nullptr;
};
}  // namespace link_creation_agent