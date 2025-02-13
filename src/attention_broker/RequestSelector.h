#ifndef _ATTENTION_BROKER_SERVER_REQUESTSELECTOR_H
#define _ATTENTION_BROKER_SERVER_REQUESTSELECTOR_H

#include "HebbianNetwork.h"
#include "SharedQueue.h"

namespace attention_broker_server {
using namespace std;
using namespace commons;

enum class SelectorType {
    EVEN_THREAD_COUNT
};

enum class RequestType {
    STIMULUS, 
    CORRELATION
};

/**
 * Abstract class used in WorkerThreads to select the next request to be processed among
 * the available request queues.
 *
 * Concrete subclasses may implement different selection algorithms based in different criteria.
 */
class RequestSelector {

public:

    virtual ~RequestSelector(); /// Destructor.

    /**
     * Factory method.
     *
     * Factory method to instantiate concrete subclasses according to the passed parameter.
     *
     * @param instance_type Type of concrete subclass to be instantiated.
     * @param thread_id ID of the thread asking for a new request.
     * @param stimulus Queue of "stimulate" requests.
     * @param correlation Queue of "correlate" requests.
     *
     * @return An object of the passed type.
     */
    static RequestSelector *factory(
        SelectorType instance_type, 
        unsigned int thread_id, 
        SharedQueue *stimulus, 
        SharedQueue *correlation);

    /**
     * Return the next request to be processed by the caller worker thread.
     *
     * @return the next request to be processed by the caller worker thread.
     */
    virtual pair<RequestType, void *> next() = 0;

protected:

    RequestSelector(unsigned int thread_id, SharedQueue *stimulus, SharedQueue *correlation); /// Basic constructor.

    unsigned int thread_id;
    SharedQueue *stimulus;
    SharedQueue *correlation;
};

/**
 * Concrete implementation of RequestSelector which evenly distribute worker threads among each type of request.
 *
 * This selector keeps half of the working threads working only in "correlate" requests and the other
 * half working only in "stimulate" requests.
 */
class EvenThreadCount : public RequestSelector {

public:

    ~EvenThreadCount(); /// Destructor.
    EvenThreadCount(unsigned int thread_id, SharedQueue *stimulus, SharedQueue *correlation); /// Basic constructor.

    /**
     * Return the next request to be processed by the caller worker thread.
     *
     * @return the next request to be processed by the caller worker thread.
     */
    pair<RequestType, void *> next();

private:

    bool even_thread_id;
};

} // namespace attention_broker_server

#endif // _ATTENTION_BROKER_SERVER_REQUESTSELECTOR_H
