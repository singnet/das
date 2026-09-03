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

    vector<QueryAnswerElement> target_elements;
    vector<QueryAnswerElement> strength_elements;
    StrengthComposition strenth_composition;
};

} // namespace link_creators
