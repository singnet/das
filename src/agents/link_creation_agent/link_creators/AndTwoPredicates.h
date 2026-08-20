#pragma once
#include <set>
#include <string>
#include "LinkCreator.h"
#include "Hasher.h"

using namespace std;

namespace link_creators {

/**
 *
 */
class AndTwoPredicates : public LinkCreator {

public:

    AndTwoPredicates();
    ~AndTwoPredicates();

    LinkCreationStats create(shared_ptr<QueryAnswer> query_answer);

private:

    static string LOGICAL_AND_HANDLE;
    static string EVALUATION_HANDLE;

    void extract_mentioned_predicates(set<string>& mentioned, const string& handle);

};

} // namespace link_creators
