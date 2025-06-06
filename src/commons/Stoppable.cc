#include "Stoppable.h"

using namespace commons;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

Stoppable::Stoppable() {
    this->stop_flag = false;
}

Stoppable::~Stoppable() {
    if (! check_and_set_stopped()) {
        stop();
    }
}

// -------------------------------------------------------------------------------------------------
// Public methods

void Stoppable::stop() { 
    if (check_and_set_stopped()) {}
}

bool Stoppable::stopped() {
    lock_guard<mutex> semaphore(this->stop_flag_mutex);
    return this->stop_flag;
}

bool Stoppable::check_and_set_stopped() {
    lock_guard<mutex> semaphore(this->stop_flag_mutex);
    bool answer = this->stop_flag;
    if (! this->stop_flag) {
        this->stop_flag = true;
    }
    return answer;
}

