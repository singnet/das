#include "StoppableThread.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace commons;

// -------------------------------------------------------------------------------------------------
// Public methods

StoppableThread::StoppableThread(const string &id) {
    LOG_DEBUG("Creating StoppableThread: " << id);
    this->stop_flag = false;
    this->id = id;
}

StoppableThread::~StoppableThread() {
    stop();
}

void StoppableThread::run(thread *thread_object) {
    this->thread_object = thread_object;
}

void StoppableThread::stop() {
    LOG_DEBUG("Stopping thread: " << this->id);
    this->stop_flag_mutex.lock();
    this->stop_flag = true;
    this->stop_flag_mutex.unlock();

    LOG_DEBUG("Joining thread: " << this->id);
    this->thread_object->join();
    LOG_DEBUG("Thread joined: " << this->id << ". destroying it.");
    delete this->thread_object;
}

bool StoppableThread::stopped() {
    this->stop_flag_mutex.lock();
    bool answer = this->stop_flag;
    this->stop_flag_mutex.unlock();
    return answer;
}
