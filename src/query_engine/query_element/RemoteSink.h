#ifndef _QUERY_ELEMENT_REMOTESINK_H
#define _QUERY_ELEMENT_REMOTESINK_H

#include <vector>

#include "QueryAnswerProcessor.h"
#include "SharedQueue.h"
#include "Sink.h"

using namespace std;

namespace query_element {

template <class AnswerType>
class RemoteSinkDeleter {
   public:
    void operator()(RemoteSink<AnswerType>* remote_sink) {
        remote_sink->graceful_shutdown();
        delete remote_sink;
    }

   private:
    stack<;
};

/**
 * A special sink which forwards the query results to a remote QueryElement (e.g. a RemoteIterator).
 */
template <class AnswerType>
class RemoteSink : public Sink<AnswerType> {
   public:
    /**
     * Constructor.
     *
     * @param precedent QueryElement just below in the query tree.
     * @param query_answer_processors List of processors to be applied to the query answers.
     * @param delete_precedent_on_destructor If true, the destructor of this QueryElement will
     *        also destruct the passed precedent QueryElement (defaulted to false).
     */
    RemoteSink(QueryElement* precedent,
               vector<unique_ptr<QueryAnswerProcessor>>&& query_answer_processors,
               bool delete_precedent_on_destructor = false);

    /**
     * Destructor.
     */
    ~RemoteSink();

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    /**
     * Gracefully shuts down the queue processor thread and the remote communication QueryNodes
     * present in this QueryElement.
     */
    virtual void graceful_shutdown();

   private:
    thread* queue_processor;
    vector<unique_ptr<QueryAnswerProcessor>> query_answer_processors;

    void queue_processor_method();
};

}  // namespace query_element

#include "RemoteSink.cc"

#endif  // _QUERY_ELEMENT_REMOTESINK_H
