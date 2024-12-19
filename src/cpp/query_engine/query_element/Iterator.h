#ifndef _QUERY_ELEMENT_ITERATOR_H
#define _QUERY_ELEMENT_ITERATOR_H

#include "Sink.h"
#include "QueryAnswer.h"

using namespace std;
using namespace query_engine;

namespace query_element {

/**
 * Concrete Sink that provides an iterator API to give access to the query answers.
 *
 * NB This is not a std::iterator as the behavior we'd expect of a std::iterator
 * doesn't fit well with the asynchronous nature of QueryElement processing.
 *
 * Instead, this class provides only two methods: one to pop and return the next
 * query answers and another to check if more answers can still be expected.
 *
 */
class Iterator : public Sink {

public:

    /**
     * Constructor expects that the QueryElement below in the tree is already constructed.
     */
    Iterator(QueryElement *precedent, bool delete_precedent_on_destructor = false);
    ~Iterator();

    // --------------------------------------------------------------------------------------------
    // Public Iterator API

    /**
     * Return true when all query answers has been processed AND all the query answers
     * that reached this QueryElement has been pop'ed out using the method pop().
     *
     * @return true iff all query answers has been processed AND all the query answers
     * that reached this QueryElement has been pop'ed out using the method pop().
     */
    bool finished();

    /**
     * Return the next query answer or NULL if none are currently available.
     *
     * NB a NULL return DOESN'T mean that the query answers are over. It means that there
     * are no query answers available now. Because of the asynchronous nature of QueryElement
     * processing, more query answers can arrive later.
     *
     * @return the next query answer or NULL if none are currently available.
     */
    QueryAnswer *pop();
};

} // namespace query_element

#endif // _QUERY_ELEMENT_ITERATOR_H
