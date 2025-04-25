#include "UniqueAssignmentFilter.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace query_element;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

UniqueAssignmentFilter::UniqueAssignmentFilter(const shared_ptr<QueryElement>& input)
    : Operator<1>({input}) {
    initialize(input);
}

UniqueAssignmentFilter::~UniqueAssignmentFilter() { stop(); }

void UniqueAssignmentFilter::initialize(const shared_ptr<QueryElement>& input) {
    this->id = "UniqueAssignmentFilter(" + input->id + ")";
    LOG_INFO(this->id);
}

// -------------------------------------------------------------------------------------------------
// QueryElement API

void UniqueAssignmentFilter::setup_buffers() {
    Operator<1>::setup_buffers();
    this->operator_thread = make_shared<StoppableThread>(this->id);
    this->operator_thread->attach(new thread(&UniqueAssignmentFilter::thread_filter, this, shared_ptr<StoppableThread> monitor));
}

void UniqueAssignmentFilter::stop() {
    if (! stopped()) {
        Operator<1>::stop();
    }
}

// -------------------------------------------------------------------------------------------------
// Private methods

void UniqueAssignmentFilter::thread_filter(shared_ptr<StoppableThread> monitor) {
    unordered_set<Assignment> already_used;

    while (true) {
        if (this->input_buffer[0]->is_query_answers_finished() &&
            this->input_buffer[0]->is_query_answers_empty()) {
            this->output_buffer->query_answers_finished();
            break;
        }
        QueryAnswer* answer = dynamic_cast<QueryAnswer*>(this->input_buffer[0]->pop_query_answer());
        if (answer != NULL) {
            if (already_used.find(answer->assignment) == already_used.end()) {
                // New assignment. Let the QueryAnswer pass.
                already_used.insert(answer->assignment);
                this->output_buffer->add_query_answer(answer);
            } else {
                // Assignment has already been processed. Delete duplicate QueryAnswer.
                delete answer;
            }
        } else {
            Utils::sleep();
        }
    }
}
