#ifndef _QUERY_ELEMENT_REMOTESINK_H
#define _QUERY_ELEMENT_REMOTESINK_H

#include <type_traits>
#include <vector>

#include "QueryAnswerProcessor.h"
#include "SharedQueue.h"
#include "Sink.h"
#include "Worker.h"

using namespace std;

namespace query_element {

template <class T, typename enable_if<is_base_of<Worker, T>::value, bool>::type = true>
class LazyWorkerDeleter {
   public:
    LazyWorkerDeleter()
        : shutting_down(false),
          objects_deleter_thread(make_unique<thread>(&LazyWorkerDeleter::objects_deleter_method, this)) {
    }

    ~LazyWorkerDeleter() {
        this->shutting_down = true;
        this->objects_deleter_thread->join();
        this->objects_deleter_thread.reset();
        lock_guard<mutex> lock(this->objects_mutex);
        T* obj;
        while (!this->objects.empty()) {
            obj = this->objects.front();
            this->objects.erase(this->objects.begin());
            delete obj;
        }
    }

    void add(T* obj) {
        lock_guard<mutex> lock(this->objects_mutex);
        if (!this->shutting_down) this->objects.push_back(obj);
    }

   private:
    vector<T*> objects;
    mutex objects_mutex;
    unique_ptr<thread> objects_deleter_thread;
    bool shutting_down;

    void objects_deleter_method() {
        T* obj;
        while (!this->shutting_down) {
            {
                lock_guard<mutex> lock(this->objects_mutex);
                while (!this->objects.empty()) {
                    obj = this->objects.front();
                    if (obj->is_work_done()) {
                        this->objects.erase(this->objects.begin());
                        delete obj;
                    }
                }
            }
            Utils::sleep(5000);
        }
    }
};

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

    bool is_work_done();

   private:
    thread* queue_processor;
    vector<unique_ptr<QueryAnswerProcessor>> query_answer_processors;

    void queue_processor_method();
};

static LazyWorkerDeleter<RemoteSink<HandlesAnswer>> remote_sink_handles_answer_deleter;

}  // namespace query_element

#include "RemoteSink.cc"

#endif  // _QUERY_ELEMENT_REMOTESINK_H
