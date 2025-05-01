#include "QueryElement.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace query_element;

// ------------------------------------------------------------------------------------------------
// Constructors and destructors

QueryElement::QueryElement() {
    this->flow_finished = false;
    this->is_terminal = false;
}

QueryElement::~QueryElement() { stop(); }

void QueryElement::stop() {
    if (check_and_set_stopped()) {}
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
