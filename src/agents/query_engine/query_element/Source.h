#pragma once

#include "QueryAnswer.h"
#include "QueryElement.h"

using namespace std;

namespace query_element {

/**
 * Superclass for elements that represent leaves in the query tree of QueryElement.
 *
 * Source adds the required DistributedAlgorithmNode (actually a specialized version of it
 * named QueryNode) and exposes a public API to interact with it transparently. Basically,
 * a client version of QueryNode (i.e. a ClientQueryNode) is setup to communicate with
 * a remote ServerQueryNode which is located in the QueryElement just above in the query tree.
 */
class Source : public QueryElement {
   public:
    /**
     * Basic empty constructor.
     */
    Source();

    /**
     * Destructor.
     */
    virtual ~Source();

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    /**
     * Gracefully shuts down the QueryNode.
     */
    virtual void graceful_shutdown();

    /**
     * Setup a ClientQueryNode to communicate with the upper QueryElement.
     */
    virtual void setup_buffers();

   protected:
    shared_ptr<QueryNode> output_buffer;
    shared_ptr<QueryElement> subsequent;
};

}  // namespace query_element
