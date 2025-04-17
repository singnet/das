#pragma once

#include "Operator.h"

using namespace std;

namespace query_element {

/**
 */
class FilterVariableEquivalence : public Operator<1> {
   public:
    // --------------------------------------------------------------------------------------------
    // Constructors and destructors

    /**
     * Constructor.
     *
     * @param input Input QueryElement which is supposed to be a Source or an Operator
     */
    FilterVariableEquivalence(const shared_ptr<QueryElement>& input);

    /**
     * Destructor.
     */
    virtual ~FilterVariableEquivalence();

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    virtual void setup_buffers();
    virtual void graceful_shutdown();

    // --------------------------------------------------------------------------------------------
    // Private stuff

   private:
    thread* operator_thread;

    void initialize(const shared_ptr<QueryElement>& input);
    void thread_filter();
};

}  // namespace query_element
