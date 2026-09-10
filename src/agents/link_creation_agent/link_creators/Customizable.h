#pragma once
#include <vector>
#include "LinkCreator.h"

using namespace std;

namespace link_creators {

/**
 *
 */
class Customizable : public LinkCreator {

public:

    enum StrengthComposition { UNDEFINED = 0, PRODUCT };

    Customizable();
    ~Customizable();

    LinkCreationStats create(shared_ptr<QueryAnswer> query_answer);

private:

    class LinkSpecification {
        vector<QueryAnswerElement> target_elements;
        vector<QueryAnswerElement> strength_elements;
        StrengthComposition strength_composition;
        string link_type;
    };

    vector<LinkSpecification> link_specification;

public:

    void add_link_specification(const vector<QueryAnswerElement>& target_elements,
                                const vector<QueryAnswerElement>& strength_elements,
                                StrengthComposition strength_composition,
                                const string& link_type) {
                                    link_specification.emplace_back(target_elements,
                                                                    strength_elements,
                                                                    strength_composition,
                                                                    link_type);
                                }
};

} // namespace link_creators
