#pragma once

#include <set>
#include <string>
#include <vector>
#include "BusCommandProxy.h"

using namespace std;

namespace service_bus {

/**
 * Bus element responsible for processing of one or more bus commands.
 *
 * Concrete subclasses must provide concrete implementation of run_command(), which
 * is the method called when one of the taken commands are issued in the bus.
 *
 * Whenever a new command processor is implemented, it's required to implement a
 * concrete subclass of BusCommandProxy as well (or reuse some other concrete
 * subclass). Anyway, the processor is required to define which subclass to use as
 * it must implement the virtual method factory_empty_proxy() which is supposed to
 * return an empty object of the proper BusCommandProxy.
 */
class BusCommandProcessor {
    friend class ServiceBus;

   public:
    /**
     * Basic constructor which expects the set of commands whose ownership will be taken.
     *
     * @param commands Set of commands owned by the BusCommandProcessor
     */
    BusCommandProcessor(const set<string>& commands);

    /**
     * Desctructor.
     */
    virtual ~BusCommandProcessor() {}

    // ---------------------------------------------------------------------------------------------
    // Virtual API which need to be implemented in concrete subclasses.

    /**
     * Returns an empty instance of the BusCommandProxy required to issue the command.
     *
     * @return An empty instance of the BusCommandProxy required to issue the command.
     */
    virtual shared_ptr<BusCommandProxy> factory_empty_proxy() = 0;

    /**
     * Method which is called when a command owned by the processor is issued in the bus.
     */
    virtual void run_command(shared_ptr<BusCommandProxy> proxy) = 0;

   private:
    bool check_command(const string& command);

    set<string> commands;
};

}  // namespace service_bus
