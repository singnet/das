#ifndef _QUERY_ELEMENT_REMOTEITERATOR_H
#define _QUERY_ELEMENT_REMOTEITERATOR_H

#include "QueryElement.h"

using namespace std;

namespace query_element {

/**
 * A special case of QueryElement because RemoteIterator is not actually an element of the
 * query tree itself but rather a utility class used to remotely connect to the sink of a query
 * tree (RemoteSink).
 *
 * Basically, the goal of this class is to allow a caller to request a query execution remotely
 * and iterate through the results using the RemoteIterator.
 *
 * NB Like Iterator in this same package, this is not a std::iterator as the behavior we'd expect
 * of a std::iterator doesn't fit well with the asynchronous nature of QueryElement processing.
 * Instead, this class provides only two methods: one to pop and return the next
 * query answers and another to check if more answers can still be expected.
 */
class RemoteIterator : public QueryElement {

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
    QueryAnswer *pop();

private:

    shared_ptr<QueryNode> remote_input_buffer;
    string local_id;
};

} // namespace query_element

#endif // _QUERY_ELEMENT_REMOTEITERATOR_H
