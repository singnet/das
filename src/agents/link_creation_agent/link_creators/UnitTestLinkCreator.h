#pragma once

#include "UnitTestLinkCreator.h"

namespace link_creators {

/**
 * UnitTestLinkCreator used in unit tests.
 */
class UnitTestLinkCreator : public LinkCreator {
   public:
    UnitTestLinkCreator() {}
    ~UnitTestLinkCreator() {}

    virtual pair<unsigned int, unsigned int> create(shared_ptr<QueryAnswer> query_answer) {
        // Returns the (string) <size, size> of the first handle.
        unsigned int n = query_answer->get(0).size();
        return make_pair(n, n);
    }
};

}  // namespace link_creators
