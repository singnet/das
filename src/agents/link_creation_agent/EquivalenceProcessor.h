/**
 * @file EquivalenceProcessor.h
 */

#pragma once
#include "Link.h"
#include "LinkProcessor.h"
#include "LCALink.h"

using namespace query_engine;
using namespace atoms;
using namespace std;
namespace link_creation_agent {
class EquivalenceProcessor : public LinkProcessor {
   public:
    EquivalenceProcessor();
    vector<vector<string>> process(shared_ptr<QueryAnswer> query_answer,
                                   optional<vector<string>> extra_params = nullopt) override;
    vector<shared_ptr<Link>> process_query(shared_ptr<QueryAnswer> query_answer,
                 optional<vector<string>> extra_params = nullopt);

   static LinkSchema build_pattern_query(const string& handle1,
                                         const string& handle2);

    ~EquivalenceProcessor() = default;

   private:
    static vector<string> get_pattern_query(const vector<string>& c1, const vector<string>& c2);
    static vector<string> get_tokenized_atom(const string& handle);
    static vector<string> count_query_template_1;
    static vector<string> count_query_template_2;
    static LCALink build_link(const string& link_type,
                              vector<LinkTargetTypes> targets,
                              vector<CustomField> custom_fields);
};
}  // namespace link_creation_agent
