#include "ThreadPool.h"
#include "Utils.h"

using namespace processor;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Public methods

ThreadPool::ThreadPool(const string& id, unsigned int num_threads) : Processor(id) {
    this->num_threads = num_threads;
    this->active_tasks = 0;
}

ThreadPool::~ThreadPool() {
}

void ThreadPool::setup() {
    Processor::setup();
}

void ThreadPool::start() {
    for (unsigned int i = 0; i < this->num_threads; ++i) {
        this->workers.emplace_back([this, i] {
            while (true) {
                function<void()> task;
                {
                    unique_lock<mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] { return this->stop_flag || !this->tasks.empty(); });
                    if (this->stop_flag && this->tasks.empty()) return;

                    task = move(this->tasks.front());
                    this->tasks.pop();
                    ++this->active_tasks;
                }
                task();
                {
                    unique_lock<mutex> lock(this->queue_mutex);
                    --this->active_tasks;
                    if (this->tasks.empty() && this->active_tasks == 0) {
                        this->done_condition.notify_all();
                    }
                }
            }
        });
    }
    Processor::start();
}

void ThreadPool::stop() {
    {
        unique_lock<mutex> lock(queue_mutex);
        this->stop_flag = true;
    }
    condition.notify_all();
    for (thread& worker : workers) worker.join();
    Processor::stop();
}

void ThreadPool::enqueue(function<void()> task) {
    if (is_running()) {
        {
            unique_lock<mutex> lock(queue_mutex);
            tasks.emplace(move(task));
        }
        condition.notify_one();
    } else {
        Utils::error("Attempt to add a job to ThreadPool " + this->to_string() + " which has not being started");
    }
}

int ThreadPool::size() {
    unique_lock<mutex> lock(queue_mutex);
    return tasks.size();
}

bool ThreadPool::empty() {
    unique_lock<mutex> lock(queue_mutex);
    return tasks.empty();
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(queue_mutex);
    done_condition.wait(lock, [this] { return tasks.empty() && active_tasks == 0; });
}
