#include "RemoteCountIterator.h"

using namespace query_element;

// ------------------------------------------------------------------------------------------------
// Constructors and destructors

RemoteCountIterator::RemoteCountIterator(const string &local_id) {
    this->local_id = local_id;
    setup_buffers();
}

RemoteCountIterator::~RemoteCountIterator() {
    graceful_shutdown();
}

// -------------------------------------------------------------------------------------------------
// Public methods

void RemoteCountIterator::setup_buffers() {
    this->remote_input_buffer = shared_ptr<QueryNode>(new QueryNodeServer(
        this->local_id,
        MessageBrokerType::GRPC));
}

void RemoteCountIterator::graceful_shutdown() {
    this->remote_input_buffer->graceful_shutdown();
}

bool RemoteCountIterator::finished() {
    // The order of the AND clauses below matters
    return (
        this->remote_input_buffer->is_query_answers_finished() &&
        this->remote_input_buffer->is_query_answers_empty());
}

CountAnswer *RemoteCountIterator::pop() {
    return (CountAnswer *) this->remote_input_buffer->pop_query_answer();
}
