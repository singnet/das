#pragma once
#include <vector>

#include "LinkCreator.h"

using namespace std;

namespace link_creators {

/**
 *
 */
class CustomizableLinkCreator : public LinkCreator {
   public:
    enum StrengthComposition { UNDEFINED = 0, PRODUCT };

    CustomizableLinkCreator();
    ~CustomizableLinkCreator();

    LinkCreationStats create(shared_ptr<QueryAnswer> query_answer);
    virtual void extra_parameters(const string& extra_parameters);

   private:
    class LinkSpecification {
       public:
        LinkSpecification() = default;
        LinkSpecification(const vector<QueryAnswerElement>& target_elements,
                          const vector<QueryAnswerElement>& strength_elements,
                          StrengthComposition strength_composition,
                          string link_type) {
            this->target_elements = target_elements;
            this->strength_elements = strength_elements;
            this->strength_composition = strength_composition;
            this->link_type = link_type;
        }
        vector<QueryAnswerElement> target_elements;
        vector<QueryAnswerElement> strength_elements;
        StrengthComposition strength_composition;
        string link_type;
    };

    vector<LinkSpecification> link_specification;

    double compute_strength(const vector<double>& components, StrengthComposition composition);

   public:
    void tokenize(vector<string>& tokens);
    void untokenize(vector<string>& tokens);
    void add_link_specification(const vector<QueryAnswerElement>& target_elements,
                                const vector<QueryAnswerElement>& strength_elements,
                                StrengthComposition strength_composition,
                                const string& link_type);
};

}  // namespace link_creators
