/**
 * @file MettaTemplateProcessor.h
 * @brief Metta Template Processor class
 */

#pragma once
#include "Link.h"
#include "LinkProcessor.h"

using namespace std;
using namespace atoms;

namespace link_creation_agent {
class MettaTemplateProcessor : public LinkProcessor {
   public:
    MettaTemplateProcessor() = default;
    vector<shared_ptr<Link>> process_query(shared_ptr<QueryAnswer> query_answer,
                                           optional<vector<string>> extra_params = nullopt) override;
    ~MettaTemplateProcessor() = default;
};
}  // namespace link_creation_agent