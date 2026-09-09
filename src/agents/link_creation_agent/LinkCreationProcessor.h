#pragma once

#include <memory>
#include <thread>

#include "BusCommandProcessor.h"
#include "LinkCreationProxy.h"
#include "PatternMatchingQueryProxy.h"
#include "StoppableThread.h"

using namespace std;
using namespace service_bus;

namespace link_creation_agent {

/**
 * Bus element responsible for processing LINK_CREATION commands.
 */
class LinkCreationProcessor : public BusCommandProcessor {
   public:
    LinkCreationProcessor();
    ~LinkCreationProcessor();

    // ---------------------------------------------------------------------------------------------
    // Virtual BusCommandProcessor API

    /**
     * Returns an empty instance of the LinkCreationProxy.
     *
     * @return An empty instance of the LinkCreationProxy.
     */
    virtual shared_ptr<BusCommandProxy> factory_empty_proxy();

    /**
     * Method which is called when a command owned by this processor is issued in the bus.
     */
    virtual void run_command(shared_ptr<BusCommandProxy> proxy);

   private:
    shared_ptr<PatternMatchingQueryProxy> issue_link_creation_query(shared_ptr<LinkCreationProxy> proxy);
    void thread_process_one_query(shared_ptr<StoppableThread>, shared_ptr<LinkCreationProxy> proxy);
    void remove_processor_thread(const string& stoppable_thread_id);
    void link_creation(shared_ptr<StoppableThread> monitor, shared_ptr<LinkCreationProxy> proxy);
    bool limit_reached(shared_ptr<LinkCreationProxy> proxy, unsigned int count, string& property);

    map<string, shared_ptr<StoppableThread>> processor_threads;
    mutex thread_management_mutex;
    shared_ptr<LinkCreationProxy> proxy;
};

}  // namespace link_creation_agent
