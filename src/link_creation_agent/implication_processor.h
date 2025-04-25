/**
 * @file implication_processor.h
 */

#pragma once
#include "link.h"
#include "link_processor.h"

using namespace query_engine;
using namespace std;

namespace link_creation_agent {
class ImplicationProcessor : public LinkProcessor {
   public:
    ImplicationProcessor();
    vector<vector<string>> process(shared_ptr<QueryAnswer> query_answer,
                                   optional<vector<string>> extra_params = nullopt) override;

   private:
    // static vector<string> get_satisfying_set_query(const string& p1,
    //                                                          const string& p2);
    static vector<string> get_pattern_query(const vector<string>& p);
    static vector<string> get_satisfying_set_query(const vector<string>& p1, const vector<string>& p2);
    static vector<string> get_tokenized_atom(const string& handle);
    static Link build_link(const string& link_type,
                           vector<LinkTargetTypes> targets,
                           vector<CustomField> custom_fields);
};
}  // namespace link_creation_agent