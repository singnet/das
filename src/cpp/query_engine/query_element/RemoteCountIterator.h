#ifndef _QUERY_ELEMENT_REMOTECOUNTITERATOR_H
#define _QUERY_ELEMENT_REMOTECOUNTITERATOR_H

#include "CountAnswer.h"
#include "QueryElement.h"

using namespace std;

namespace query_element {

class RemoteCountIterator : public QueryElement {
   public:
    RemoteCountIterator(const string& local_id);

    ~RemoteCountIterator();

    virtual void graceful_shutdown();

    virtual void setup_buffers();

    bool finished();

    CountAnswer* pop();

   private:
    shared_ptr<QueryNode> remote_input_buffer;
    string local_id;
};

}  // namespace query_element

#endif  // _QUERY_ELEMENT_REMOTECOUNTITERATOR_H
