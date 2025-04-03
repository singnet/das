#pragma once

#include "Iterator.h"

using namespace query_element;

// -------------------------------------------------------------------------------------------------
// Public methods

template <class AnswerType>
Iterator<AnswerType>::Iterator(shared_ptr<QueryElement> precedent)
    : Sink<AnswerType>(precedent, "Iterator(" + precedent->id + ")") {}

template <class AnswerType>
Iterator<AnswerType>::~Iterator() {}

template <class AnswerType>
bool Iterator<AnswerType>::finished() {
    // The order of the AND clauses below matters
    return (this->input_buffer->is_query_answers_finished() &&
            this->input_buffer->is_query_answers_empty());
}

template <class AnswerType>
QueryAnswer* Iterator<AnswerType>::pop() {
    return (QueryAnswer*) this->input_buffer->pop_query_answer();
}
