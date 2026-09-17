#pragma once
#include <vector>

#include "LinkCreator.h"
#include "PatternMatchingQueryProxy.h"

using namespace std;
using namespace query_engine;

namespace link_creators {

/**
 *
 */
class CustomizableLinkCreator : public LinkCreator {
   public:
    enum StrengthComposition {
        UNDEFINED = 0,
        PRODUCT,
        INTERSECTION_OVER_UNION,
        INTERSECTION_OVER_A,
        INTERSECTION_OVER_B
    };
    static char EXTRA_PARAMETERS_SPLIT_CHAR;

    CustomizableLinkCreator();
    ~CustomizableLinkCreator();

    LinkCreationStats create(shared_ptr<QueryAnswer> query_answer);
    virtual void extra_parameters(const string& extra_parameters);

   private:
    class LinkSpecification {
       public:
        LinkSpecification(const vector<QueryAnswerElement>& target_elements,
                          const vector<QueryAnswerElement>& strength_elements,
                          string link_type,
                          StrengthComposition strength_composition,
                          const vector<string> queries);
        vector<QueryAnswerElement> target_elements;
        vector<QueryAnswerElement> strength_elements;
        string link_type;
        StrengthComposition strength_composition;
        vector<string> queries;

       private:
        void check();
    };

    vector<LinkSpecification> link_specification;

    void insert_or_update(map<string, double>& count_map, const string& key, double value);
    shared_ptr<PatternMatchingQueryProxy> issue_link_count_query(const string& query_str);
    void compute_counts(shared_ptr<QueryAnswer> query_answer,
                        LinkSpecification& spec,
                        double& count_A,
                        double& count_B,
                        double& count_intersection,
                        double& count_union);
    double compute_strength(shared_ptr<QueryAnswer> query_answer, LinkSpecification& spec);

   public:
    void tokenize(vector<string>& tokens);
    void untokenize(vector<string>& tokens);
    void add_link_specification(const vector<QueryAnswerElement>& target_elements,
                                const vector<QueryAnswerElement>& strength_elements,
                                const string& link_type,
                                StrengthComposition strength_composition,
                                const vector<string>& queries = {});
};

}  // namespace link_creators
