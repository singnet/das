#ifndef _SERVICE_BUS_SERVICEBUSSINGLETON_H
#define _SERVICE_BUS_SERVICEBUSSINGLETON_H

#include <memory>
#include "ServiceBus.h"

using namespace std;

namespace service_bus {

/**
 *
 */
class ServiceBusSingleton {

public:

    ~ServiceBusSingleton() {};
    static void init(const string& host_id, const string& known_peer = "");
    static shared_ptr<ServiceBus> get_instance();


private:

    ServiceBusSingleton() {};
    static bool initialized;
    static shared_ptr<ServiceBus> service_bus;
};

} // namespace service_bus

#endif // _SERVICE_BUS_SERVICEBUSSINGLETON_H
