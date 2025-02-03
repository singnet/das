#include "RemoteSink.h"

#include "QueryAnswer.h"

using namespace query_element;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

RemoteSink::RemoteSink(QueryElement* precedent,
                       const vector<QueryAnswerProcessor*> query_answer_processors,
                       bool delete_precedent_on_destructor)
    : Sink(precedent, "RemoteSink(" + precedent->id + ")", delete_precedent_on_destructor, true) {
#ifdef DEBUG
    cout << "RemoteSink::RemoteSink() BEGIN" << endl;
#endif

    this->queue_processor = NULL;
    for (auto processor : query_answer_processors) {
        this->query_answer_processors.emplace_back(unique_ptr<QueryAnswerProcessor>(processor));
    }
    this->queue_processor = new thread(&RemoteSink::queue_processor_method, this);
#ifdef DEBUG
    cout << "RemoteSink::RemoteSink() END" << endl;
#endif
}

RemoteSink::~RemoteSink() { graceful_shutdown(); }

// -------------------------------------------------------------------------------------------------
// Public methods

void RemoteSink::graceful_shutdown() {
#ifdef DEBUG
    cout << "RemoteSink::graceful_shutdown() BEGIN" << endl;
#endif
    Sink::graceful_shutdown();
    set_flow_finished();
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

void RemoteSink::queue_processor_method() {
#ifdef DEBUG
    cout << "RemoteSink::queue_processor_method() BEGIN" << endl;
#endif
    do {
        if (is_flow_finished() || (this->input_buffer->is_query_answers_finished() &&
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
    set_flow_finished();
    for (const auto& processor : this->query_answer_processors) {
        processor->query_answers_finished();
    }
#ifdef DEBUG
    cout << "RemoteSink::queue_processor_method() END" << endl;
#endif
}
