#pragma once
#include <set>
#include <string>

#include "Hasher.h"
#include "LinkCreator.h"

using namespace std;

namespace link_creators {

/**
 * This LinkCreator expects a query answer with three assigned variables:
 * PREDICATE1, PREDICATE2 and CONCEPT and adds a LOGICAL_AND Evaluation expression
 * pointing to them.
 *
 * For instance: (Evaluation (LogicalAnd $PREDICATE1 $PREDICATE2) $CONCEPT)
 *
 * The strength of the new link is calculated as the product of the strengths of
 * all handles in the query_answer->get_handles_vector().
 */
class AndTwoPredicates : public LinkCreator {
   public:
    AndTwoPredicates();
    ~AndTwoPredicates();

    /**
     * Creates two expressions: (LogicalAnd $PREDICATE1 $PREDICATE2) and
     * (Evaluation (LogicalAnd $PREDICATE1 $PREDICATE2) $CONCEPT) given a query_answer
     * with proper variable assignment.
     *
     * @param query_answer A QueryAnswer with assignment for PREDICATE1, PREDICATE2 and CONCEPT
     */
    LinkCreationStats create(shared_ptr<QueryAnswer> query_answer);

   private:
    static string LOGICAL_AND_HANDLE;
    static string EVALUATION_HANDLE;

    void extract_mentioned_predicates(set<string>& mentioned, const string& handle);
};

}  // namespace link_creators
