#include "QueryElement.h"

using namespace query_element;

// ------------------------------------------------------------------------------------------------
// Constructors and destructors

QueryElement::QueryElement() {
    this->flow_finished = false;
    this->is_terminal = false;
    this->is_operator = false;
    this->arity = 0;
}

QueryElement::~QueryElement() {}

// ------------------------------------------------------------------------------------------------
// Public methods

string QueryElement::to_string() { return ""; }

// ------------------------------------------------------------------------------------------------
// Protected methods

void QueryElement::set_flow_finished() {
    this->flow_finished_mutex.lock();
    this->flow_finished = true;
    this->flow_finished_mutex.unlock();
}

bool QueryElement::is_flow_finished() {
    bool answer;
    this->flow_finished_mutex.lock();
    answer = this->flow_finished;
    this->flow_finished_mutex.unlock();
    return answer;
}
