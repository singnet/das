#pragma once
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>

using namespace std;

template <typename T>
class Queue {
   private:
    queue<T> m_queue;
    mutex m_mutex;
    condition_variable m_cond;

   public:
    void enqueue(T item) {
        unique_lock<mutex> lock(m_mutex);
        m_queue.push(item);
        m_cond.notify_one();
    }

    T dequeue() {
        unique_lock<mutex> lock(m_mutex);
        m_cond.wait(lock, [this]() { return !m_queue.empty(); });
        T item = m_queue.front();
        m_queue.pop();
        return item;
    }

    bool empty() {
        bool answer;
        unique_lock<mutex> lock(m_mutex);
        answer = m_queue.empty();
        m_cond.notify_one();
        return answer;
    }
};