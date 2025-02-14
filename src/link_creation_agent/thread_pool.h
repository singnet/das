/**
 * @file thread_pool.h
 * @brief Thread pool class
 */
#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

/**
 * @brief Thread pool class
 */

using namespace std;
class ThreadPool {
   public:
    /**
     * @brief Constructor
     */
    explicit ThreadPool(size_t threads) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    /**
     * @brief Destructor
     */
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) worker.join();
    }

    /**
     * @brief Enqueue a task to the thread pool
     * @param task Task to be enqueued
     */
    void enqueue(function<void()> task) {
        {
            unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(move(task));
        }
        condition.notify_one();
    }

   private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop = false;
};
