/**
 * @file TemplateProcessor.h
 * @brief Template Processor class
 */

#pragma once
#include "Link.h"
#include "LinkProcessor.h"

using namespace std;
using namespace atoms;

namespace link_creation_agent {
class LinkTemplateProcessor : public LinkProcessor {
   public:
    LinkTemplateProcessor() = default;
    vector<shared_ptr<Link>> process_query(shared_ptr<QueryAnswer> query_answer,
                                           optional<vector<string>> extra_params = nullopt) override;
    ~LinkTemplateProcessor() = default;
};
}  // namespace link_creation_agent