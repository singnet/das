#pragma once

#include "QueryElement.h"

using namespace std;

namespace query_element {

/**
 * Superclass for elements that represent the root in a query tree of QueryElement.
 *
 * It's a "sink" in the sense of being an element where the flow of links stops, going
 * nowhere further.
 *
 * Sink adds the required DistributedAlgorithmNode (actually a specialized version of it
 * named QueryNode) and exposes a public API to interact with it transparently. Basically,
 * a server version of QueryNode (i.e. a ServerQueryNode) is setup to communicate with
 * a remote ClientQueryNode which is located in the QueryElement just below in the query tree.
 */
class Sink : public QueryElement {
   public:
    /**
     * Constructor expects that the QueryElement below in the tree is already constructed.
     *
     * @param precedent QueryElement just below in the query tree.
     * @param id Unique id for this QueryElement.
     * @param setup_buffers_flag If true, the setup_buffers() method is called in the constructor.
     */
    Sink(shared_ptr<QueryElement> precedent, const string& id, bool setup_buffers_flag = true);

    /**
     * Destructor.
     */
    virtual ~Sink();

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    /**
     * Gracefully shuts down the QueryNode.
     */
    virtual void graceful_shutdown();

    /**
     * Setup a ServerQueryNode to communicate with one or more QueryElement just below in the
     * query tree.
     */
    virtual void setup_buffers();

    shared_ptr<QueryNode> input_buffer;

   protected:
    shared_ptr<QueryElement> precedent;
};

}  // namespace query_element
