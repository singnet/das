#include "RemoteIterator.h"

using namespace query_element;

// ------------------------------------------------------------------------------------------------
// Constructors and destructors

RemoteIterator::RemoteIterator(const string &local_id) {
    this->local_id = local_id;
    setup_buffers();
}

RemoteIterator::~RemoteIterator() {
    graceful_shutdown();
}

// -------------------------------------------------------------------------------------------------
// Public methods

void RemoteIterator::setup_buffers() {
    this->remote_input_buffer = shared_ptr<QueryNode>(new QueryNodeServer(
        this->local_id,
        MessageBrokerType::GRPC));
}

void RemoteIterator::graceful_shutdown() {
    this->remote_input_buffer->graceful_shutdown();
}

bool RemoteIterator::finished() {
    // The order of the AND clauses below matters
    return (
        this->remote_input_buffer->is_query_answers_finished() &&
        this->remote_input_buffer->is_query_answers_empty());
}

QueryAnswer *RemoteIterator::pop() {
    return (QueryAnswer *) this->remote_input_buffer->pop_query_answer();
}
