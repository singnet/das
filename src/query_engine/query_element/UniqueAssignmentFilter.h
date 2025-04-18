#pragma once

#include "Operator.h"

using namespace std;

namespace query_element {

/**
 * This QueryElement acts like an unary operator which filters out QueryAnswers withthe exact
 * same assignments as a previously processed one.
 *
 * This element is added to the query tree by the query command processor, i.e., there are
 * no tokens associated with it to explicitly put it in a query. The caller is supposed to
 * build a proxy object setting an unique assignment flag in proxy's constructor.
 */
class UniqueAssignmentFilter : public Operator<1> {
   public:
    // --------------------------------------------------------------------------------------------
    // Constructors and destructors

    /**
     * Constructor.
     *
     * @param input Input QueryElement which is supposed to be a Source or an Operator
     */
    UniqueAssignmentFilter(const shared_ptr<QueryElement>& input);

    /**
     * Destructor.
     */
    virtual ~UniqueAssignmentFilter();

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    /**
     * Setup a ServerQueryNode to communicate with one or more QueryElement just below in the
     * query tree.
     */
    virtual void setup_buffers();

    /**
     * Gracefully shuts down the QueryNode.
     */
    virtual void graceful_shutdown();

    // --------------------------------------------------------------------------------------------
    // Private stuff

   private:
    thread* operator_thread;

    void initialize(const shared_ptr<QueryElement>& input);
    void thread_filter();
};

}  // namespace query_element
