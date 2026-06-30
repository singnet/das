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
    {
        // Set the flag under the lock, but join OUTSIDE the lock so the thread being joined can
        // still call stopped() without deadlocking.
        lock_guard<mutex> guard(this->stop_flag_mutex);
        if (this->stop_flag) {
            return;  // already stopped or detached
        }
        this->stop_flag = true;
    }
    if (this->thread_object == NULL) {
        return;  // nothing attached
    }
    if (join_thread) {
        LOG_DEBUG("Joining thread: " << this->id);
        this->thread_object->join();
        LOG_DEBUG("Thread joined: " << this->id << ". destroying it.");
        delete this->thread_object;
        this->thread_object = NULL;
    }
}

void StoppableThread::detach() {
    {
        lock_guard<mutex> guard(this->stop_flag_mutex);
        if (this->stop_flag) {
            return;  // already stopped or detached
        }
        this->stop_flag = true;
    }
    if (this->thread_object != NULL) {
        LOG_DEBUG("Detaching thread: " << this->id);
        // detach() releases the OS thread; deleting the std::thread handle afterwards does not
        // affect the still-running OS thread.
        this->thread_object->detach();
        delete this->thread_object;
        this->thread_object = NULL;
    }
}

bool StoppableThread::stopped() {
    this->stop_flag_mutex.lock();
    bool answer = this->stop_flag;
    this->stop_flag_mutex.unlock();
    return answer;
}

string StoppableThread::get_id() { return this->id; }
