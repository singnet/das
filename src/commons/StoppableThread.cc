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
    // stop_flag only means "stop requested" (it is what stopped() reports). It is intentionally
    // independent of whether the underlying thread has been reaped, so requesting a stop without
    // joining (stop(false)) still leaves the thread to be reaped by a later stop()/detach()/dtor.
    thread* to_reap = NULL;
    {
        lock_guard<mutex> guard(this->stop_flag_mutex);
        this->stop_flag = true;
        if (join_thread) {
            // Claim the thread exactly once: whoever swaps the pointer out reaps it; concurrent or
            // later callers see NULL and skip.
            to_reap = this->thread_object;
            this->thread_object = NULL;
        }
    }
    // Join OUTSIDE the lock so the thread being joined can still call stopped() without deadlocking.
    if (to_reap != NULL) {
        LOG_DEBUG("Joining thread: " << this->id);
        to_reap->join();
        LOG_DEBUG("Thread joined: " << this->id << ". destroying it.");
        delete to_reap;
    }
}

void StoppableThread::detach() {
    thread* to_reap = NULL;
    {
        lock_guard<mutex> guard(this->stop_flag_mutex);
        this->stop_flag = true;
        // Claim the thread exactly once (see stop()).
        to_reap = this->thread_object;
        this->thread_object = NULL;
    }
    if (to_reap != NULL) {
        LOG_DEBUG("Detaching thread: " << this->id);
        // detach() releases the OS thread; deleting the std::thread handle afterwards does not
        // affect the still-running OS thread.
        to_reap->detach();
        delete to_reap;
    }
}

bool StoppableThread::stopped() {
    this->stop_flag_mutex.lock();
    bool answer = this->stop_flag;
    this->stop_flag_mutex.unlock();
    return answer;
}

string StoppableThread::get_id() { return this->id; }
