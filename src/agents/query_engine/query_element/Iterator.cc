#include "Iterator.h"

using namespace query_element;

// -------------------------------------------------------------------------------------------------
// Public methods

Iterator::Iterator(shared_ptr<QueryElement> precedent)
    : Sink(precedent, "Iterator(" + precedent->id + ")") {}

Iterator::~Iterator() {}

bool Iterator::finished() { return this->input_buffer->answers_finished_and_empty(); }

QueryAnswer* Iterator::pop() { return (QueryAnswer*) this->input_buffer->pop_query_answer(); }
