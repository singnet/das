/**
 * @file EquivalenceProcessor.h
 */

#pragma once
#include "Link.h"
#include "LinkProcessor.h"
#include "LinkSchema.h"

using namespace query_engine;
using namespace atoms;
using namespace std;
namespace link_creation_agent {
class EquivalenceProcessor : public LinkProcessor {
   public:
    EquivalenceProcessor();
    vector<shared_ptr<Link>> process_query(shared_ptr<QueryAnswer> query_answer,
                                           optional<vector<string>> extra_params = nullopt);

    static LinkSchema build_pattern_query(const string& handle1, const string& handle2);

    ~EquivalenceProcessor() = default;
};
}  // namespace link_creation_agent
