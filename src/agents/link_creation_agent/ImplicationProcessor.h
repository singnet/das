/**
 * @file ImplicationProcessor.h
 */

#pragma once
#include "Link.h"
#include "LinkProcessor.h"
#include "LinkSchema.h"

using namespace query_engine;
using namespace atoms;
using namespace std;

namespace link_creation_agent {
class ImplicationProcessor : public LinkProcessor {
   public:
    ImplicationProcessor();
    vector<shared_ptr<Link>> process_query(shared_ptr<QueryAnswer> query_answer,
                                           optional<vector<string>> extra_params = nullopt) override;

    static LinkSchema build_pattern_query(const string& handle);
    static LinkSchema build_satisfying_set_query(const string& p1_handle, const string& p2_handle);
    static bool link_exists(const string& handle1, const string& handle2);
};
}  // namespace link_creation_agent