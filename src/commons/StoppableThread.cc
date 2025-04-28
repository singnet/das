#include "StoppableThread.h"

#include "Utils.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace commons;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

StoppableThread::StoppableThread(const string& id) {
    this->stop_flag = false;
    this->id = id;
    this->string_representation = "Thread<" + id + ">";
    this->thread_object = NULL;
    LOG_DEBUG("Creating StoppableThread: " << this->to_string());
}

StoppableThread::~StoppableThread() { 
    stop(); 
    delete this->thread_object;
}

// -------------------------------------------------------------------------------------------------
// Public methods

void StoppableThread::attach(thread* thread_object) { this->thread_object = thread_object; }

void StoppableThread::stop() { this->stop(true); }

void StoppableThread::stop(bool join_thread) {
    if (this->thread_object == NULL) {
        Utils::error("No thread attached to StoppableThread: " + this->to_string());
    } else {
        this->stop_flag_mutex.lock();
        if (! this->stop_flag) {
            LOG_DEBUG("Stopping thread: " << this->to_string());
            this->stop_flag = true;
            if (join_thread) {
                LOG_DEBUG("Joining thread: " << this->to_string());
                this->stop_flag_mutex.unlock();
                this->thread_object->join();
                LOG_DEBUG("Thread joined: " << this->to_string());
            } else {
                this->stop_flag_mutex.unlock();
            }
        } else {
            this->stop_flag_mutex.unlock();
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

const string& StoppableThread::to_string() { return this->string_representation; }
