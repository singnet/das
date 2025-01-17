#pragma once
#include "das_query_node.h"

using namespace std;
using namespace query_node;

namespace query_element {

class RemoteIterator  {

public:
    
    /**
     * Constructor.
     *
     * @param local_id The id of this element in the network which connects to the RemoteSink.
     * Typically is something like "host:port".
     */
    RemoteIterator(const string &local_id);

    /**
     * Destructor.
     */
    ~RemoteIterator();

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    virtual void graceful_shutdown();
    virtual void setup_buffers();

    // --------------------------------------------------------------------------------------------
    // Iterator API

    /**
     * Return true when all query answers has been processed AND all the query answers
     * that reached this QueryElement has been pop'ed out using the method pop().
     *
     * @return true iff all query answers has been processed AND all the query answers
     * that reached this QueryElement has been pop'ed out using the method pop().
     */
    bool finished();

    /**
     * Return the next query answer or NULL if none are currently available.
     *
     * NB a NULL return DOESN'T mean that the query answers are over. It means that there
     * are no query answers available now. Because of the asynchronous nature of QueryElement
     * processing, more query answers can arrive later.
     *
     * @return the next query answer or NULL if none are currently available.
     */
    // QueryAnswer *pop();

private:

    shared_ptr<QueryNode> remote_input_buffer;
    string local_id;
};

} // namespace query_element
