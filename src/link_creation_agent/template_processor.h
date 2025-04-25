/**
 * @file template_processor.h
 * @brief Template Processor class
 */

#pragma once
#include "link.h"
#include "link_processor.h"

using namespace std;

namespace link_creation_agent {
class LinkTemplateProcessor : public LinkProcessor {
   public:
    LinkTemplateProcessor() = default;
    vector<vector<string>> process(shared_ptr<QueryAnswer> query_answer,
                                   optional<vector<string>> extra_params = nullopt) override;
    ~LinkTemplateProcessor() = default;

   private:
    shared_ptr<Link> process_template_request(shared_ptr<QueryAnswer> query_answer,
                                              LinkCreateTemplate& link_template);
};
}  // namespace link_creation_agent