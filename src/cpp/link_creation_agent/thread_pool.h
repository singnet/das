/**
 * @brief Example of thread pool implementation
 */
#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <iostream>


/**
 * @brief Thread pool class example
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t thread_count) : stop_(false) {};
    ~ThreadPool();
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
};
