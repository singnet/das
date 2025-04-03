#pragma once

#include <memory>
#include <thread>
#include "BusCommandProcessor.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryElement.h"

using namespace std;

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

    void setup_query_tree();
    void thread_process_one_query();

    shared_ptr<PatternMatchingQueryProxy> proxy;
    shared_ptr<QueryElement> root_query_element;
    thread* query_processor;
};

} // namespace atomdb
