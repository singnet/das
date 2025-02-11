#include "RequestQueue.h"

using namespace commons;

// --------------------------------------------------------------------------------
// Public methods

RequestQueue::RequestQueue(unsigned int initial_size) {
    size = initial_size;
    requests = new void*[size];
    count = 0;
    start = 0;
    end = 0;
}

RequestQueue::~RequestQueue() {
    delete [] requests;
}

bool RequestQueue::empty() {
    bool answer;
    request_queue_mutex.lock();
    answer = (count == 0);
    request_queue_mutex.unlock();
    return answer;
}

void RequestQueue::enqueue(void *request) {
    request_queue_mutex.lock();
    if (count == size) {
        enlarge_request_queue();
    }
    requests[end] = request;
    end = (end + 1) % size;
    count++;
    request_queue_mutex.unlock();
}

void *RequestQueue::dequeue() {
    void *answer = NULL;
    request_queue_mutex.lock();
    if (count > 0) {
        answer = requests[start];
        start = (start + 1) % size;
        count--;
    }
    request_queue_mutex.unlock();
    return answer;
}

// --------------------------------------------------------------------------------
// Protected methods

unsigned int RequestQueue::current_size() {
    return size;
}

unsigned int RequestQueue::current_start() {
    return start;
}

unsigned int RequestQueue::current_end() {
    return end;
}

unsigned int RequestQueue::current_count() {
    return count;
}

// --------------------------------------------------------------------------------
// Private methods

void RequestQueue::enlarge_request_queue() {
    unsigned int _new_size = size * 2;
    void **_new_queue = new void*[_new_size];
    unsigned int _cursor = start;
    unsigned int _new_cursor = 0;
    do {
        _new_queue[_new_cursor++] = requests[_cursor];
        _cursor = (_cursor + 1) % size;
    } while (_cursor != end);
    size = _new_size;
    start = 0;
    end = _new_cursor;
    delete [] requests;
    requests = _new_queue;
    // count remains unchanged
}
