#ifndef _COMMONS_SHAREDQUEUE_H
#define _COMMONS_SHAREDQUEUE_H

#include <chrono>
#include <condition_variable>
#include <functional>
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
    SharedQueue(unsigned int initial_size = 1000);  // Basic constructor

    ~SharedQueue();                                 /// Destructor.

    /**
     * Enqueues a request and notifies one waiting consumer.
     *
     * @param request Request to be queued.
     */
    void enqueue(void* request);

    /**
     * Dequeues a request (non-blocking). Returns NULL if the queue is empty.
     *
     * @return The dequeued request or NULL.
     */
    void* dequeue();

    /**
     * Blocks until an item is available or the timeout expires.
     * Returns NULL on timeout.
     *
     * @param timeout_ms Maximum time to wait in milliseconds.
     * @return The dequeued request or NULL on timeout.
     */
    void* blocking_dequeue(unsigned int timeout_ms = 500);

    /**
     * Dequeues and applies deleter to every remaining item. Useful for cleanup
     * when the queue may still hold heap-allocated objects.
     *
     * @param deleter Called once per remaining item.
     */
    void drain(std::function<void(void*)> deleter);

    /**
     * Returns true iff the queue is empty.
     *
     * @return true iff the queue is empty.
     */
    bool empty();

    /**
     * Returns the number of elements in the queue.
     *
     * @return The number of elements in the queue.
     */
    unsigned int size();

   protected:
    unsigned int current_size();
    unsigned int current_start();
    unsigned int current_end();
    unsigned int current_count();

   private:
    std::mutex shared_queue_mutex;
    std::condition_variable not_empty_cv;

    void** requests;  // GRPC documentation states that request types should not be inherited
    unsigned int allocated_size;
    unsigned int count;
    unsigned int start;
    unsigned int end;

    void enlarge_shared_queue();

    void* dequeue_locked();
};

}  // namespace commons

#endif  // _COMMONS_SHAREDQUEUE_H
