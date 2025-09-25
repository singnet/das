#pragma once

#include <map>
#include <memory>
#include <thread>

#include "BusCommandProcessor.h"
#include "ContextBrokerProxy.h"
#include "StoppableThread.h"

using namespace std;
using namespace service_bus;

namespace context_broker {

/**
 * Bus element responsible for processing CONTEXT_BROKER commands.
 */
class ContextBrokerProcessor : public BusCommandProcessor {
   public:
    ContextBrokerProcessor();
    ~ContextBrokerProcessor();

    // ---------------------------------------------------------------------------------------------
    // Virtual BusCommandProcessor API

    /**
     * Returns an empty instance of the ContextBrokerProxy.
     *
     * @return An empty instance of the ContextBrokerProxy.
     */
    virtual shared_ptr<BusCommandProxy> factory_empty_proxy();

    /**
     * Method which is called when a command owned by this processor is issued in the bus.
     */
    virtual void run_command(shared_ptr<BusCommandProxy> proxy);

    /**
     * Method which is called to create a context.
     *
     * @param monitor The monitor object.
     * @param proxy The proxy object.
     * @return The context hash key.
     */
    string create_context(shared_ptr<StoppableThread> monitor, shared_ptr<ContextBrokerProxy> proxy);

    /**
     * Write cache.
     * @param proxy The proxy object.
     */
    void write_cache(shared_ptr<ContextBrokerProxy> proxy);

    /**
     * Read cache.
     * @param proxy The proxy object.
     * @return True if cache was read successfully, false otherwise
     */
    bool read_cache(shared_ptr<ContextBrokerProxy> proxy);

    /**
     * Set AttentionBroker parameters.
     * @param proxy The proxy object.
     */
    void set_attention_broker_parameters(shared_ptr<ContextBrokerProxy> proxy);

    /**
     * Update attention broker.
     * @param proxy The proxy object.
     */
    void update_attention_broker(shared_ptr<ContextBrokerProxy> proxy);

   private:
    void thread_process_one_query(shared_ptr<StoppableThread> monitor,
                                  shared_ptr<ContextBrokerProxy> proxy);

    map<string, shared_ptr<StoppableThread>> query_threads;
    mutex query_threads_mutex;
    shared_ptr<ContextBrokerProxy> proxy;
};

}  // namespace context_broker
