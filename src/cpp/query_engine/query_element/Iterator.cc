#include "Iterator.h"

using namespace query_element;

// -------------------------------------------------------------------------------------------------
// Public methods

Iterator::Iterator(
    QueryElement *precedent, 
    bool delete_precedent_on_destructor) : 
    Sink(precedent, "Iterator(" + precedent->id + ")", delete_precedent_on_destructor) {
}

Iterator::~Iterator() {
}

bool Iterator::finished() {
    // The order of the AND clauses below matters
    return (
        this->input_buffer->is_query_answers_finished() &&
        this->input_buffer->is_query_answers_empty());
}

QueryAnswer *Iterator::pop() {
    return (QueryAnswer *) this->input_buffer->pop_query_answer();
}
