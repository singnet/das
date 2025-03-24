#ifndef _SERVICE_BUS_SERVICEBUS_H
#define _SERVICE_BUS_SERVICEBUS_H

#include <set>
#include <string>

using namespace std;

namespace service_bus {

/**
 *
 */
class ServiceBus {

public:

    ServiceBus();
    ~ServiceBus();

    static set<string> SERVICE_LIST;
    static void initialize_statics() {
        SERVICE_LIST.insert("PATTERN_MATCHING_QUERY");
    }

private:

};

} // namespace service_bus

#endif // _SERVICE_BUS_SERVICEBUS_H
