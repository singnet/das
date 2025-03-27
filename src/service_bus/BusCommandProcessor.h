#ifndef _SERVICE_BUS_BUSCOMMANDPROCESSOR_H
#define _SERVICE_BUS_BUSCOMMANDPROCESSOR_H

#include<set>
#include<vector>
#include<string>

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
    // Virtual API which need to be iomplemented in concrete subclasses.

    virtual void run_command(const string& command,const  vector<string>& args) = 0;


private:

    bool check_command(const string& command);

    set<string> commands;
};

} // namespace service_bus

#endif // _SERVICE_BUS_BUSCOMMANDPROCESSOR_H
