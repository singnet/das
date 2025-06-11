#pragma once

#include <map>
#include <memory>
#include <thread>

#include "BusCommandProcessor.h"
#include "QueryEvolutionProxy.h"
#include "StoppableThread.h"

#define MAX_BUNDLE_SIZE ((unsigned int) 10000)

using namespace std;
using namespace service_bus;

namespace evolution {

/**
 * Bus element responsible for processing QUERY_EVOLUTION commands.
 */
class QueryEvolutionProcessor : public BusCommandProcessor {
   public:
    QueryEvolutionProcessor();
    ~QueryEvolutionProcessor();

    // ---------------------------------------------------------------------------------------------
    // Virtual BusCommandProcessor API

    /**
     * Returns an empty instance of the QueryEvolutionProxy.
     *
     * @return An empty instance of the QueryEvolutionProxy.
     */
    virtual shared_ptr<BusCommandProxy> factory_empty_proxy();

    /**
     * Method which is called when a command owned by this processor is issued in the bus.
     */
    virtual void run_command(shared_ptr<BusCommandProxy> proxy);

   private:
    void thread_process_one_query(shared_ptr<StoppableThread>, shared_ptr<QueryEvolutionProxy> proxy);

    void remove_query_thread(const string& stoppable_thread_id);
    map<string, shared_ptr<StoppableThread>> query_threads;
    mutex query_threads_mutex;
    shared_ptr<QueryEvolutionProxy> proxy;
};

}  // namespace evolution
