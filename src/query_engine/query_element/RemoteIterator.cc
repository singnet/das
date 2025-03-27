#pragma once

#include "RemoteIterator.h"

using namespace query_element;

// ------------------------------------------------------------------------------------------------
// Constructors and destructors
template <class AnswerType>
RemoteIterator<AnswerType>::RemoteIterator(const string& local_id) {
    this->local_id = local_id;
    this->setup_buffers();
}

template <class AnswerType>
RemoteIterator<AnswerType>::~RemoteIterator() {
    this->graceful_shutdown();
}

// -------------------------------------------------------------------------------------------------
// Public methods

template <class AnswerType>
void RemoteIterator<AnswerType>::setup_buffers() {
    this->remote_input_buffer =
        make_shared<QueryNodeServer<AnswerType>>(this->local_id, MessageBrokerType::GRPC);
}

template <class AnswerType>
void RemoteIterator<AnswerType>::graceful_shutdown() {
    this->remote_input_buffer->graceful_shutdown();
}

template <class AnswerType>
bool RemoteIterator<AnswerType>::finished() {
    // The order of the AND clauses below matters
    return (this->remote_input_buffer->is_query_answers_finished() &&
            this->remote_input_buffer->is_query_answers_empty());
}

template <class AnswerType>
QueryAnswer* RemoteIterator<AnswerType>::pop() {
    return (QueryAnswer*) this->remote_input_buffer->pop_query_answer();
}

template <class AnswerType>
string RemoteIterator<AnswerType>::get_local_id() const {
    return this->local_id;
}
