#ifndef _SERVICE_BUS_BUSCOMMANDPROCESSOR_H
#define _SERVICE_BUS_BUSCOMMANDPROCESSOR_H

#include<set>
#include<vector>
#include<string>

using namespace std;

namespace service_bus {

/**
 *
 */
class BusCommandProcessor {
    friend class ServiceBus;

public:

    BusCommandProcessor(const set<string>& commands);
    virtual ~BusCommandProcessor() {}

    bool check_command(const string& command);
    virtual void run_command(const string& command,const  vector<string>& args) = 0;

private:

    set<string> commands;
};

} // namespace service_bus

#endif // _SERVICE_BUS_BUSCOMMANDPROCESSOR_H
