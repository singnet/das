/**
 * @file template_processor.h
 * @brief Template Processor class
 */

#pragma once
#include "link.h"
#include "link_processor.h"

namespace link_creation_agent {
class LinkTemplateProcessor : public LinkProcessor {
   public:
    LinkTemplateProcessor() = default;
    std::vector<std::vector<std::string>> process(
        shared_ptr<QueryAnswer> query_answer,
        std::optional<std::vector<std::string>> extra_params = nullopt) override;
    ~LinkTemplateProcessor() = default;

   private:
    shared_ptr<Link> process_template_request(shared_ptr<QueryAnswer> query_answer,
                                              LinkCreateTemplate& link_template);
};
}  // namespace link_creation_agent