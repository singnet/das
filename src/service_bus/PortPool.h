#pragma once

#include "Utils.h"
#include "SharedQueue.h"

using namespace std;
using namespace commons;

namespace service_bus {

/**
 *
 */
class PortPool {

public:

    static void initialize_statics(unsigned long port_lower, unsigned long port_upper) {
        if (port_lower > port_upper) {
            Utils::error("Invalid port limits [" + to_string(port_lower) + ".." +
                         to_string(port_upper) + "]");
        }

        PORT_LOWER = port_lower;
        PORT_UPPER = port_upper;
        POOL = new SharedQueue(port_upper - port_lower + 1);
        for (unsigned long port = port_lower; port <= port_upper; port++) {
            POOL->enqueue((void*) port);
        }
    }

    static unsigned long get_port() {
        unsigned long port = (unsigned long) POOL->dequeue();
        if (! port) {
            Utils::error("Unable to get available PORT number in [" +
            to_string(PORT_LOWER) + ".." + to_string(PORT_UPPER) + "]");
        }
        return port;
    }

    static void return_port(unsigned long port) {
        POOL->enqueue((void *) port);
    }

    ~PortPool() {}

private:

    PortPool();
    static SharedQueue* POOL;
    static unsigned long PORT_LOWER;
    static unsigned long PORT_UPPER;
};

} // namespace service_bus
