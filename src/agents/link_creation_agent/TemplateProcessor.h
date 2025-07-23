/**
 * @file TemplateProcessor.h
 * @brief Template Processor class
 */

#pragma once
#include "Link.h"
#include "LCALink.h"
#include "LinkProcessor.h"


using namespace std;
using namespace atoms;

namespace link_creation_agent {
class LinkTemplateProcessor : public LinkProcessor {
   public:
    LinkTemplateProcessor() = default;
    vector<vector<string>> process(shared_ptr<QueryAnswer> query_answer,
                                   optional<vector<string>> extra_params = nullopt) override;
    vector<shared_ptr<Link>> process_query(shared_ptr<QueryAnswer> query_answer,
                   optional<vector<string>> extra_params = nullopt) override;
    ~LinkTemplateProcessor() = default;

   private:
    shared_ptr<LCALink> process_template_request(shared_ptr<QueryAnswer> query_answer,
                                                 LinkCreateTemplate& link_template);
};
}  // namespace link_creation_agent