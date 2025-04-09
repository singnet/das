#pragma once

#include "RemoteSink.h"

#include "QueryAnswer.h"

using namespace query_element;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

template <class AnswerType>
RemoteSink<AnswerType>::RemoteSink(shared_ptr<QueryElement> precedent,
                                   vector<unique_ptr<QueryAnswerProcessor>>&& query_answer_processors)
    : Sink<AnswerType>(precedent, "RemoteSink(" + precedent->id + ")", true),
      queue_processor(new thread(&RemoteSink::queue_processor_method, this)),
      query_answer_processors(move(query_answer_processors)) {}

template <class AnswerType>
RemoteSink<AnswerType>::~RemoteSink() {
#ifdef DEBUG
    cout << "RemoteSink::~RemoteSink() BEGIN" << endl;
#endif
    graceful_shutdown();
#ifdef DEBUG
    cout << "RemoteSink::~RemoteSink() END" << endl;
#endif
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
        delete this->queue_processor;
        this->queue_processor = NULL;
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
    cout << "RemoteSink::queue_processor_method() 1" << endl;
        if (this->is_flow_finished() || (this->input_buffer->is_query_answers_finished() &&
                                         this->input_buffer->is_query_answers_empty())) {
            break;
        }
    cout << "RemoteSink::queue_processor_method() 2" << endl;
        bool idle_flag = true;
        QueryAnswer* qa;
    cout << "RemoteSink::queue_processor_method() 3" << endl;
        while ((qa = dynamic_cast<QueryAnswer*>(this->input_buffer->pop_query_answer())) != NULL) {
    cout << "RemoteSink::queue_processor_method() 4" << endl;
            for (const auto& processor : this->query_answer_processors) {
    cout << "RemoteSink::queue_processor_method() 5" << endl;
                processor->process_answer(qa);
    cout << "RemoteSink::queue_processor_method() 6" << endl;
            }
    cout << "RemoteSink::queue_processor_method() 7" << endl;
            idle_flag = false;
    cout << "RemoteSink::queue_processor_method() 8" << endl;
        }
    cout << "RemoteSink::queue_processor_method() 9" << endl;
        if (idle_flag) {
            Utils::sleep();
        }
    cout << "RemoteSink::queue_processor_method() 10" << endl;
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
