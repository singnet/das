#include "StoppableThread.h"

#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace commons;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

StoppableThread::StoppableThread(const string& id) {
    LOG_DEBUG("Creating StoppableThread: " << id);
    this->stop_flag = false;
    this->id = id;
    this->thread_object = NULL;
}

StoppableThread::~StoppableThread() { stop(); }

// -------------------------------------------------------------------------------------------------
// Public methods

void StoppableThread::attach(thread* thread_object) { this->thread_object = thread_object; }

void StoppableThread::stop() { this->stop(true); }

void StoppableThread::stop(bool join_thread) {
    if (this->thread_object == NULL) {
        Utils::error("No thread attached to StoppableThread: " + this->id);
    } else if (!this->stop_flag) {
        LOG_DEBUG("Stopping thread: " << this->id);
        this->stop_flag_mutex.lock();
        this->stop_flag = true;
        this->stop_flag_mutex.unlock();
        if (join_thread) {
            LOG_DEBUG("Joining thread: " << this->id);
            this->thread_object->join();
            LOG_DEBUG("Thread joined: " << this->id << ". destroying it.");
            delete this->thread_object;
        }
    }
}

bool StoppableThread::stopped() {
    this->stop_flag_mutex.lock();
    bool answer = this->stop_flag;
    this->stop_flag_mutex.unlock();
    return answer;
}

string StoppableThread::get_id() { return this->id; }
