#pragma once

#include <memory>
#include <thread>
#include <stack>
#include "BusCommandProcessor.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryElement.h"

using namespace std;
using namespace service_bus;
using namespace query_element;

namespace atomdb {

/**
 *
 */
class PatternMatchingQueryProcessor : public BusCommandProcessor {

public:

    PatternMatchingQueryProcessor();
    ~PatternMatchingQueryProcessor();

    // ---------------------------------------------------------------------------------------------
    // Virtual BusCommandProcessor API

    /**
     * Returns an empty instance of the PatternMatchingQueryProxy.
     *
     * @return An empty instance of the PatternMatchingQueryProxy.
     */
    virtual shared_ptr<BusCommandProxy> factory_empty_proxy();

    /**
     * Method which is called when a command owned by this processor is issued in the bus.
     */
    virtual void run_command(shared_ptr<BusCommandProxy> proxy);

private:

    shared_ptr<QueryElement> setup_query_tree(shared_ptr<PatternMatchingQueryProxy> proxy);
    void thread_process_one_query(shared_ptr<PatternMatchingQueryProxy> proxy);
    shared_ptr<QueryElement> build_link_template(shared_ptr<PatternMatchingQueryProxy> proxy,
                                                 unsigned int cursor,
                                                 stack<shared_ptr<QueryElement>>& element_stack);

    shared_ptr<QueryElement> build_and(shared_ptr<PatternMatchingQueryProxy> proxy,
                                       unsigned int cursor,
                                       stack<shared_ptr<QueryElement>>& element_stack);

    shared_ptr<QueryElement> build_or(shared_ptr<PatternMatchingQueryProxy> proxy,
                                      unsigned int cursor,
                                      stack<shared_ptr<QueryElement>>& element_stack);

    shared_ptr<QueryElement> build_link(shared_ptr<PatternMatchingQueryProxy> proxy,
                                        unsigned int cursor,
                                        stack<shared_ptr<QueryElement>>& element_stack);

    vector<thread*> query_threads;
    mutex query_threads_mutex;
    shared_ptr<PatternMatchingQueryProxy> proxy;
};

} // namespace atomdb
