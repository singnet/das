#ifndef _QUERY_ELEMENT_SINK_H
#define _QUERY_ELEMENT_SINK_H

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
     * @param delete_precedent_on_destructor If true, the destructor of this QueryElement will
     * also destruct the passed precedent QueryElement (defaulted to false).
     */
    Sink(
        QueryElement *precedent, 
        const string &id, 
        bool delete_precedent_on_destructor = false,
        bool setup_buffers_flag = true);

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
     * Setup a ServerQueryNode to commnunicate with one or more QueryElement just below in the
     * query tree.
     */
    virtual void setup_buffers();

protected:

    shared_ptr<QueryNode> input_buffer;
    QueryElement *precedent;

private:

    bool delete_precedent_on_destructor;
};

} // namespace query_element

#endif // _QUERY_ELEMENT_SINK_H
