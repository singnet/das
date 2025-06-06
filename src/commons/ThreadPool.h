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
    explicit ThreadPool(size_t threads) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this, i] {
                while (true) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;

                        task = move(tasks.front());
                        tasks.pop();
                        ++active_tasks;
                    }
                    task();
                    {
                        unique_lock<mutex> lock(queue_mutex);
                        --active_tasks;
                        if (tasks.empty() && active_tasks == 0) {
                            done_condition.notify_all();
                        }
                    }
                }
            });
        }
    }

    ~ThreadPool() {
        {
            unique_lock<mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (thread& worker : workers) worker.join();
    }

    void enqueue(function<void()> task) {
        {
            unique_lock<mutex> lock(queue_mutex);
            tasks.emplace(move(task));
        }
        condition.notify_one();
    }

    int size() {
        unique_lock<mutex> lock(queue_mutex);
        return tasks.size();
    }

    bool empty() {
        unique_lock<mutex> lock(queue_mutex);
        return tasks.empty();
    }

    void wait() {
        unique_lock<mutex> lock(queue_mutex);
        done_condition.wait(lock, [this] {
            return tasks.empty() && active_tasks == 0;
        });
    }

   private:
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex queue_mutex;
    condition_variable condition;
    condition_variable done_condition;
    size_t active_tasks = 0;
    bool stop = false;
};
