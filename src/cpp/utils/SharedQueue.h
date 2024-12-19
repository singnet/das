#ifndef _COMMONS_SHAREDQUEUE_H
#define _COMMONS_SHAREDQUEUE_H

#include <mutex>

namespace commons {

/**
 * Data abstraction of a synchronized (thread-safe) queue for assynchronous requests.
 *
 * Internally, this abstraction uses an array of requests to avoid the need to create cell
 * objects on every insertion. Because of this, on new insertions it's possible to reach queue
 * size limit during an insertion. When that happens, the array is doubled in size. Initial size
 * is passed as a constructor's parameter.
 */
class SharedQueue {

public:

    SharedQueue(unsigned int initial_size = 1000); // Basic constructor

    ~SharedQueue(); /// Destructor.

    /**
     * Enqueues a request.
     *
     * @param request Shared to be queued.
     */
    void enqueue(void *request);

    /**
     * Dequeues a request.
     *
     * @return The dequeued request.
     */
    void *dequeue();

    /**
     * Returns true iff the queue is empty.
     */
    bool empty();

protected:

    unsigned int current_size();
    unsigned int current_start();
    unsigned int current_end();
    unsigned int current_count();

private:

    std::mutex request_queue_mutex;

    void **requests; // GRPC documentation states that request types should not be inherited
    unsigned int size;
    unsigned int count;
    unsigned int start;
    unsigned int end;

    void enlarge_request_queue();
};

} // namespace commons

#endif // _COMMONS_SHAREDQUEUE_H
