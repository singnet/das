#include "PortPool.h"

using namespace service_bus;

SharedQueue* PortPool::POOL = NULL;
unsigned long PortPool::PORT_LOWER = 0;
unsigned long PortPool::PORT_UPPER = 0;
