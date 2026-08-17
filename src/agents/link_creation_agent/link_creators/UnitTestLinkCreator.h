#pragma once

#include "LinkCreator.h"

namespace link_creators {

/**
 * UnitTestLinkCreator used in unit tests.
 */
class UnitTestLinkCreator : public LinkCreator {
   public:
    UnitTestLinkCreator() {}
    ~UnitTestLinkCreator() {}

    LinkCreationStats create(shared_ptr<QueryAnswer> query_answer) {
        LinkCreationStats stats;
        if (query_answer == nullptr) {
            RAISE_ERROR("Invalid null query_answer");
        }
        unsigned int n = query_answer->get(0).size();
        stats.visited = true;
        stats.created = n;
        stats.updated = n + 1;
        return stats;
    }
};

}  // namespace link_creators
