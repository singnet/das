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
 * Subclasses must provide concrete implementation of run_command(), which
 * is the method called when one of the taken commands are issued in the bus.
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
    virtual ~BusCommandProcessor() {}

    // ---------------------------------------------------------------------------------------------
    // Virtual API which need to be implemented in concrete subclasses.

    virtual shared_ptr<BusCommandProxy> factory_empty_proxy() = 0;
    virtual void run_command(shared_ptr<BusCommandProxy> proxy) = 0;

   private:
    bool check_command(const string& command);

    set<string> commands;
};

}  // namespace service_bus
