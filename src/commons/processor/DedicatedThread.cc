#ifndef LOG_LEVEL
#define LOG_LEVEL INFO_LEVEL
#endif
#include "Logger.h"

#include "DedicatedThread.h"
#include "Utils.h"

using namespace processor;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Public methods

DedicatedThread::DedicatedThread(const string& id, ThreadMethod* job): Processor(id) {
    this->job = job;
    this->start_flag = false;
    this->stop_flag = false;
}

DedicatedThread::~DedicatedThread() {
}

void DedicatedThread::setup() {
    this->thread_object = new thread(&DedicatedThread::thread_method, this);
    Processor::setup();
}

void DedicatedThread::start() {
    if (is_setup()) {
        this->api_mutex.lock();
        this->start_flag = true;
        this->api_mutex.unlock();
    }
    Processor::start();
}

void DedicatedThread::stop() {
    this->api_mutex.lock();
    this->stop_flag = true;
    this->api_mutex.unlock();
    Processor::stop();
    LOG_DEBUG("Joining DedicatedThread " + this->to_string() + "...");
    this->thread_object->join();
    LOG_DEBUG("Joined DedicatedThread " + this->to_string() ". Deleting thread object.");
    delete this->thread_object;
}

// -------------------------------------------------------------------------------------------------
// Private methods

bool DedicatedThread::started() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->start_flag;
}

bool DedicatedThread::stopped() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->stop_flag;
}

void DedicatedThread::thread_method() {
    while (! started()) {
        Utils::sleep();
    }
    do {
        if (! this->job->thread_one_step()) {
            Utils::sleep();
        };
    } while (! stopped());
}
