#include "QueryElement.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace query_element;

// ------------------------------------------------------------------------------------------------
// Constructors and destructors

QueryElement::QueryElement() {
    this->flow_finished = false;
    this->is_terminal = false;
}

QueryElement::~QueryElement() { stop(); }

// ------------------------------------------------------------------------------------------------
// Public methods

void QueryElement::stop() {
    lock_guard<mutex> semaphore(this->stop_flag_mutex);
    LOG_DEBUG("Stopping QueryElement: " << this->id);
    this->stop_flag = true;
}

bool MessageBroker::stopped() {
    lock_guard<mutex> semaphore(this->stop_flag_mutex);
    return this->stop_flag;
}

// ------------------------------------------------------------------------------------------------
// Protected methods

void QueryElement::set_flow_finished() {
    lock_guard<mutex> semaphore(this->flow_finished_mutex);
    this->flow_finished = true;
}

bool QueryElement::is_flow_finished() {
    lock_guard<mutex> semaphore(this->flow_finished_mutex);
    return this->flow_finished;
}
