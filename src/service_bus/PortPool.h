#pragma once

#include "SharedQueue.h"
#include "Utils.h"

using namespace std;
using namespace commons;

namespace service_bus {

/**
 *
 */
class PortPool {
   public:
    static void initialize_statics(unsigned int port_lower, unsigned int port_upper);
    static unsigned int get_port();
    static void return_port(unsigned int port);

   private:
    PortPool();
    static SharedQueue* POOL;
    static unsigned int PORT_LOWER;
    static unsigned int PORT_UPPER;
};

}  // namespace service_bus
