#include "Iterator.h"

using namespace query_element;

// -------------------------------------------------------------------------------------------------
// Public methods

Iterator::Iterator(shared_ptr<QueryElement> precedent)
    : Sink(precedent, "Iterator(" + precedent->id + ")") {}

Iterator::~Iterator() {}

bool Iterator::finished() {
    // The order of the AND clauses below matters
    return (this->input_buffer->is_query_answers_finished() &&
            this->input_buffer->is_query_answers_empty());
}

QueryAnswer* Iterator::pop() {
    return (QueryAnswer*) this->input_buffer->pop_query_answer();
}
