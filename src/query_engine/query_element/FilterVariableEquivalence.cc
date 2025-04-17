#pragma once

#include "FilterVariableEquivalence.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace query_element;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

FilterVariableEquivalence::FilterVariableEquivalence(const shared_ptr<QueryElement>& input) 
    : Operator<1>({input}) { 
    initialize(input); 
}

~FilterVariableEquivalence::FilterVariableEquivalence() { graceful_shutdown(); }

void FilterVariableEquivalence::initialize(const shared_ptr<QueryElement>& input) {
    this->id = "FilterVariableEquivalence(" + input->id + ")";
    LOG_INFO(this->id);
}

// -------------------------------------------------------------------------------------------------
// QueryElement API

virtual void FilterVariableEquivalence::setup_buffers() {
    Operator<1>::setup_buffers();
    this->operator_thread = new thread(&FilterVariableEquivalence::thread_filter, this);
}

virtual void FilterVariableEquivalence::graceful_shutdown() {
    Operator<1>::graceful_shutdown();
    if (this->operator_thread != NULL) {
        this->operator_thread->join();
        delete this->operator_thread;
        this->operator_thread = NULL;
    }
}

// -------------------------------------------------------------------------------------------------
// Private methods

void FilterVariableEquivalence::thread_filter() {

    unordered_map<Assignment> already_used;
    while (true) {
        if (this->input_buffer[0]->is_query_answers_finished() &&
            this->input_buffer[0]->is_query_answers_empty()) {

            this->output_buffer->query_answers_finished();
            break;
        }
        answer = dynamic_cast<QueryAnswer*>(this->input_buffer[0]->pop_query_answer());
        if (answer != NULL) {
            if (already_used.find(answer->assignment) == already_used.end()) {
                // New assignment. Let the QueryAnswer pass.
                this->output_buffer->add_query_answer(answer);
                AQUI
            } else  {
                e AQUI
            }

        } else {
            Utils::sleep();
        }
    }
}
