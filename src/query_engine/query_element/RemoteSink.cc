#pragma once

#include "RemoteSink.h"

#include "QueryAnswer.h"

using namespace query_element;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

template <class AnswerType>
RemoteSink<AnswerType>::RemoteSink(QueryElement* precedent,
                                   vector<unique_ptr<QueryAnswerProcessor>>&& query_answer_processors,
                                   bool delete_precedent_on_destructor)
    : Sink<AnswerType>(
          precedent, "RemoteSink(" + precedent->id + ")", delete_precedent_on_destructor, true),
      queue_processor(new thread(&RemoteSink::queue_processor_method, this)),
      query_answer_processors(move(query_answer_processors)) {}

template <class AnswerType>
RemoteSink<AnswerType>::~RemoteSink() {
    graceful_shutdown();
}

// -------------------------------------------------------------------------------------------------
// Public methods

template <class AnswerType>
void RemoteSink<AnswerType>::graceful_shutdown() {
#ifdef DEBUG
    cout << "RemoteSink::graceful_shutdown() BEGIN" << endl;
#endif
    Sink<AnswerType>::graceful_shutdown();
    this->set_flow_finished();
    if (this->queue_processor != NULL) {
        this->queue_processor->join();
    }
    for (const auto& processor : this->query_answer_processors) {
        processor->graceful_shutdown();
    }
#ifdef DEBUG
    cout << "RemoteSink::graceful_shutdown() END" << endl;
#endif
}

// -------------------------------------------------------------------------------------------------
// Private methods

template <class AnswerType>
void RemoteSink<AnswerType>::queue_processor_method() {
#ifdef DEBUG
    cout << "RemoteSink::queue_processor_method() BEGIN" << endl;
#endif
    do {
        if (this->is_flow_finished() || (this->input_buffer->is_query_answers_finished() &&
                                         this->input_buffer->is_query_answers_empty())) {
            break;
        }
        bool idle_flag = true;
        QueryAnswer* qa;
        while ((qa = dynamic_cast<QueryAnswer*>(this->input_buffer->pop_query_answer())) != NULL) {
            for (const auto& processor : this->query_answer_processors) {
                processor->process_answer(qa);
            }
            idle_flag = false;
        }
        if (idle_flag) {
            Utils::sleep();
        }
    } while (true);

#ifdef DEBUG
    cout << "RemoteSink::queue_processor_method() ready to return" << endl;
#endif
    this->set_flow_finished();
    for (const auto& processor : this->query_answer_processors) {
        processor->query_answers_finished();
    }
#ifdef DEBUG
    cout << "RemoteSink::queue_processor_method() END" << endl;
#endif
}
