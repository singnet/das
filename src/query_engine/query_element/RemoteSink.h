#ifndef _QUERY_ELEMENT_REMOTESINK_H
#define _QUERY_ELEMENT_REMOTESINK_H

#include <vector>

#include "LazyWorkerDeleter.h"
#include "QueryAnswerProcessor.h"
#include "SharedQueue.h"
#include "Sink.h"

using namespace std;

namespace query_element {

/**
 * A special sink which forwards the query results to a remote QueryElement (e.g. a RemoteIterator).
 */
template <class AnswerType>
class RemoteSink : public Sink<AnswerType>, public Worker {
   public:
    /**
     * Constructor.
     *
     * @param precedent QueryElement just below in the query tree.
     * @param query_answer_processors List of processors to be applied to the query answers.
     */
    RemoteSink(shared_ptr<QueryElement> precedent,
               vector<unique_ptr<QueryAnswerProcessor>>&& query_answer_processors);

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

    virtual bool is_work_done() override {
        for (const auto& processor : this->query_answer_processors) {
            if (!processor->is_work_done()) return false;
        }
        if (!this->input_buffer->is_query_answers_empty()) return false;
        if (!this->input_buffer->is_query_answers_finished()) return false;
        if (!this->is_flow_finished()) return false;
        return true;
    }

   private:
    thread* queue_processor;
    vector<unique_ptr<QueryAnswerProcessor>> query_answer_processors;

    void queue_processor_method();
};

}  // namespace query_element

#include "RemoteSink.cc"

#endif  // _QUERY_ELEMENT_REMOTESINK_H
