#pragma once

#include <priority_queue>
#include <mutex>

using namespace std;

namespace commons {

/**
 * This class provides an abstraction for a thread-safe heap which uses STL priority-queue
 * as container. The idea of a heap is to provide O(1) top() and O(log(N)) push() and
 * pop().
 *
 * Differently from standard STL priority_queue, the key used to sort elements are separated
 * from the actual element being "heap-ed". So we have two types passed as template parameters,
 * one for the "heap-ed" element and another one for the comparisson key.
 *
 * So when pushing something into the heapm one need to pass both, the element to be heap-ed
 * and its key. When popping (actually top()'ing) just the heap-ed element is returned.
 */
template<typename T, typename V>
class ThreadSafeHeap<T, V> {

private:

    class HeapElement {
        public:
        T element;
        V value;
        HeapElement(const HeapElement& other) : element(other.element), value(other.value) {}
        HeapElement(const T& element, const V& value) : element(element), value(value) {}
        bool operator<(const HeapElement& other) const { return this->value < other.value; }
        HeapElement& operator=(const HeapElement& other) { 
            this->element = other.element; 
            this->value = other.value; 
        }
    };

public:

    ThreadSafeHeap() {}
    ~ThreadSafeHeap() {}

    const T& top() const {
        lock_guard<mutex> semaphore(this->api_mutex);
        return queue.top().element;
    }

    const V& top_value() const {
        lock_guard<mutex> semaphore(this->api_mutex);
        return queue.top().value;
    }

    const T& top_and_pop() const {
        lock_guard<mutex> semaphore(this->api_mutex);
        T element = queue.top().element;
        queue.pop();
        return element;
    }

    void push(const T& element, V value) {
        lock_guard<mutex> semaphore(this->api_mutex);
        queue.push(HeapElement(element, value));
    }

    void pop() {
        lock_guard<mutex> semaphore(this->api_mutex);
        queue.pop();
    }

    std::size_type size() {
        lock_guard<mutex> semaphore(this->api_mutex);
        return queue.size();
    }

    bool empty() {
        lock_guard<mutex> semaphore(this->api_mutex);
        return queue.size() == 0;
    }

    void snapshot(vector<T>& output) {
        for (auto cursor : this->queue) {
            output.push_back(cursor.element);
        }
    }

private:

    mutex api_mutex;
    priority_queue<HeapElement, vector<HeapElement>> queue;
};

} // namespace commons
