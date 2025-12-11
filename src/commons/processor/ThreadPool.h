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

#include "Processor.h"

using namespace std;

namespace processor {

/**
 *
 */
class ThreadPool : public Processor {
   public:
    ThreadPool(const string& id, unsigned int num_threads);
    virtual ~ThreadPool();

    virtual void setup();
    virtual void start();
    virtual void stop();

    void enqueue(function<void()> task);
    int size();
    bool empty();
    void wait();

   private:
    unsigned int num_threads;

    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex queue_mutex;
    condition_variable condition;
    condition_variable done_condition;
    unsigned int active_tasks;
    bool stop_flag = false;
};

}  // namespace processor
