#include "PortPool.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace service_bus;

SharedQueue* PortPool::POOL = NULL;
unsigned int PortPool::PORT_LOWER = 0;
unsigned int PortPool::PORT_UPPER = 0;

void PortPool::initialize_statics(unsigned int port_lower, unsigned int port_upper) {
    if (port_lower > port_upper) {
        Utils::error("Invalid port limits [" + to_string(port_lower) + ".." + to_string(port_upper) +
                     "]");
    }
    LOG_INFO("Port range: [" << port_lower << " : " << port_upper << "]");
    PORT_LOWER = port_lower;
    PORT_UPPER = port_upper;
    POOL = new SharedQueue(port_upper - port_lower + 1);
    for (unsigned int port = port_lower; port <= port_upper; port++) {
        if (Utils::is_port_available(port)) {
            POOL->enqueue((void*) ((unsigned long) port));
        } else {
            LOG_ERROR("Discarding unavailable PORT " + std::to_string(port));
        }
    }
}

unsigned int PortPool::get_port() {
    unsigned int port = (unsigned int) ((unsigned long) POOL->dequeue());
    if (!port) {
        Utils::error("Unable to get available PORT number in [" + to_string(PORT_LOWER) + ".." +
                     to_string(PORT_UPPER) + "]");
    }
    return port;
}

void PortPool::return_port(unsigned int port) { POOL->enqueue((void*) ((unsigned long) port)); }
