#include "SharedQueue.h"

using namespace commons;

// --------------------------------------------------------------------------------
// Public methods

SharedQueue::SharedQueue(unsigned int initial_size) {
    allocated_size = initial_size;
    requests = new void*[allocated_size];
    count = 0;
    start = 0;
    end = 0;
}

SharedQueue::~SharedQueue() { delete[] requests; }

unsigned int SharedQueue::size() {
    unsigned int answer;
    shared_queue_mutex.lock();
    answer = count;
    shared_queue_mutex.unlock();
    return answer;
}

bool SharedQueue::empty() {
    bool answer;
    shared_queue_mutex.lock();
    answer = (count == 0);
    shared_queue_mutex.unlock();
    return answer;
}

void SharedQueue::enqueue(void* request) {
    shared_queue_mutex.lock();
    if (count == allocated_size) {
        enlarge_shared_queue();
    }
    requests[end] = request;
    end = (end + 1) % allocated_size;
    count++;
    shared_queue_mutex.unlock();
}

void* SharedQueue::dequeue() {
    void* answer = NULL;
    shared_queue_mutex.lock();
    if (count > 0) {
        answer = requests[start];
        start = (start + 1) % allocated_size;
        count--;
    }
    shared_queue_mutex.unlock();
    return answer;
}

// --------------------------------------------------------------------------------
// Protected methods

unsigned int SharedQueue::current_size() { return allocated_size; }

unsigned int SharedQueue::current_start() { return start; }

unsigned int SharedQueue::current_end() { return end; }

unsigned int SharedQueue::current_count() { return count; }

// --------------------------------------------------------------------------------
// Private methods

void SharedQueue::enlarge_shared_queue() {
    unsigned int _new_size = allocated_size * 2;
    void** _new_queue = new void*[_new_size];
    unsigned int _cursor = start;
    unsigned int _new_cursor = 0;
    do {
        _new_queue[_new_cursor++] = requests[_cursor];
        _cursor = (_cursor + 1) % allocated_size;
    } while (_cursor != end);
    allocated_size = _new_size;
    start = 0;
    end = _new_cursor;
    delete[] requests;
    requests = _new_queue;
    // count remains unchanged
}
