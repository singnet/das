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
    static void initialize_statics(unsigned long port_lower, unsigned long port_upper);
    static unsigned long get_port();
    static void return_port(unsigned long port);

   private:
    PortPool();
    static SharedQueue* POOL;
    static unsigned long PORT_LOWER;
    static unsigned long PORT_UPPER;
};

}  // namespace service_bus
