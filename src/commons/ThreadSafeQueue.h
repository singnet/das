#pragma once

#include <mutex>
#include <queue>

#include "Utils.h"

namespace commons {

template <typename T>
class ThreadSafeQueue {
   private:
    queue<T> _queue;
    mutex api_mutex;

   public:
    ThreadSafeQueue() { lock_guard<mutex> semaphore(this->api_mutex); }

    ~ThreadSafeQueue() { lock_guard<mutex> semaphore(this->api_mutex); }

    void push(const T& element) {
        lock_guard<mutex> semaphore(this->api_mutex);
        this->_queue.push(element);
    }

    void pop() {
        lock_guard<mutex> semaphore(this->api_mutex);
        this->_queue.pop();
    }

    const T& front() {
        lock_guard<mutex> semaphore(this->api_mutex);
        return this->_queue.front();
    }

    T front_and_pop() {
        lock_guard<mutex> semaphore(this->api_mutex);
        T element = this->_queue.front();
        this->_queue.pop();
        return element;
    }

    bool empty() {
        lock_guard<mutex> semaphore(this->api_mutex);
        return this->_queue.empty();
    }

    void clear() {
        lock_guard<mutex> semaphore(this->api_mutex);
        this->_queue.clear();
    }
};
}  // namespace commons
