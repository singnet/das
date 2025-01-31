#include "RemoteCountSink.h"

#include <cstring>
#include <stack>
#include <unordered_map>

#include "CountAnswer.h"
#include "HandlesAnswer.h"

using namespace query_element;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

RemoteCountSink::RemoteCountSink(QueryElement* precedent,
                                 const string& local_id,
                                 const string& remote_id,
                                 const string& context,
                                 bool delete_precedent_on_destructor)
    : Sink(precedent, "RemoteCountSink(" + precedent->id + ")", delete_precedent_on_destructor, false) {
#ifdef DEBUG
    cout << "RemoteCountSink::RemoteCountSink() BEGIN" << endl;
    cout << "RemoteCountSink::RemoteCountSink() local_id: " << local_id << endl;
    cout << "RemoteCountSink::RemoteCountSink() remote_id: " << remote_id << endl;
#endif

    this->query_context = context;
    this->local_id = local_id;
    this->remote_id = remote_id;
    this->queue_processor = NULL;
    RemoteCountSink::setup_buffers();
    Sink::setup_buffers();
    this->queue_processor = new thread(&RemoteCountSink::queue_processor_method, this);
#ifdef DEBUG
    cout << "RemoteCountSink::RemoteCountSink() END" << endl;
#endif
}

RemoteCountSink::~RemoteCountSink() { graceful_shutdown(); }

// -------------------------------------------------------------------------------------------------
// Public methods

void RemoteCountSink::setup_buffers() {
#ifdef DEBUG
    cout << "RemoteCountSink::setup_buffers() BEGIN" << endl;
#endif
    this->remote_output_buffer = shared_ptr<QueryNode>(
        new QueryNodeClient(this->local_id, this->remote_id, MessageBrokerType::GRPC));
#ifdef DEBUG
    cout << "RemoteCountSink::setup_buffers() END" << endl;
#endif
}

void RemoteCountSink::graceful_shutdown() {
#ifdef DEBUG
    cout << "RemoteCountSink::graceful_shutdown() BEGIN" << endl;
#endif
    Sink::graceful_shutdown();
    set_flow_finished();
    if (this->queue_processor != NULL) {
        this->queue_processor->join();
    }
    this->remote_output_buffer->graceful_shutdown();
#ifdef DEBUG
    cout << "RemoteCountSink::graceful_shutdown() END" << endl;
#endif
}

// -------------------------------------------------------------------------------------------------
// Private methods

void RemoteCountSink::queue_processor_method() {
#ifdef DEBUG
    cout << "RemoteCountSink::queue_processor_method() BEGIN" << endl;
#endif

    int count = 0;
    do {
        if (is_flow_finished() || (this->input_buffer->is_query_answers_finished() &&
                                   this->input_buffer->is_query_answers_empty())) {
            break;
        }
        bool idle_flag = true;
        HandlesAnswer* handles_answer;
        while ((handles_answer = dynamic_cast<HandlesAnswer*>(this->input_buffer->pop_query_answer())) != NULL) {
            count++;
            idle_flag = false;
        }
        if (idle_flag) {
            Utils::sleep();
        }
    } while (true);

    cout << "RemoteCountSink::queue_processor_method() count: " << count << endl;

    this->remote_output_buffer->add_query_answer(new CountAnswer(count));

#ifdef DEBUG
    cout << "RemoteCountSink::queue_processor_method() ready to return" << endl;
#endif
    this->remote_output_buffer->query_answers_finished();
    set_flow_finished();
#ifdef DEBUG
    cout << "RemoteCountSink::queue_processor_method() END" << endl;
#endif
}
