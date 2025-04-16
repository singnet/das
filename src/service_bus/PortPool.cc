#include "PortPool.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace service_bus;

SharedQueue* PortPool::POOL = NULL;
unsigned long PortPool::PORT_LOWER = 0;
unsigned long PortPool::PORT_UPPER = 0;

void PortPool::initialize_statics(unsigned long port_lower, unsigned long port_upper) {
    if (port_lower > port_upper) {
        Utils::error("Invalid port limits [" + to_string(port_lower) + ".." +
                     to_string(port_upper) + "]");
    }

    LOG_INFO("Port range: [" << port_lower << " : " << port_upper << "]");
    PORT_LOWER = port_lower;
    PORT_UPPER = port_upper;
    POOL = new SharedQueue(port_upper - port_lower + 1);
    for (unsigned long port = port_lower; port <= port_upper; port++) {
        POOL->enqueue((void*) port);
    }
}

unsigned long PortPool::get_port() {
    unsigned long port = (unsigned long) POOL->dequeue();
    if (! port) {
        Utils::error("Unable to get available PORT number in [" +
        to_string(PORT_LOWER) + ".." + to_string(PORT_UPPER) + "]");
    }
    return port;
}

void PortPool::return_port(unsigned long port) {
    POOL->enqueue((void *) port);
}
