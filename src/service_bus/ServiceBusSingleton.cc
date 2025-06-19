#include "ServiceBusSingleton.h"

#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace service_bus;
using namespace commons;

bool ServiceBusSingleton::INITIALIZED = false;
shared_ptr<ServiceBus> ServiceBusSingleton::SERVICE_BUS = shared_ptr<ServiceBus>{};
mutex ServiceBusSingleton::API_MUTEX;

// -------------------------------------------------------------------------------------------------
// Public methods

void ServiceBusSingleton::init(const string& host_id,
                               const string& known_peer,
                               unsigned int port_lower,
                               unsigned int port_upper) {
    lock_guard<mutex> semaphore(API_MUTEX);
    if (INITIALIZED) {
        Utils::error(
            "ServiceBusSingleton already initialized. "
            "ServiceBusSingleton::init() should be called only once.");
    } else {
        ServiceBus::initialize_statics({}, port_lower, port_upper);
        SERVICE_BUS = shared_ptr<ServiceBus>(new ServiceBus(host_id, known_peer));
        INITIALIZED = true;
        Utils::sleep(500);
    }
}

shared_ptr<ServiceBus> ServiceBusSingleton::get_instance() {
    lock_guard<mutex> semaphore(API_MUTEX);
    if (!INITIALIZED) {
        Utils::error(
            "Uninitialized ServiceBusSingleton. ServiceBusSingleton::init() "
            "must be called before ServiceBusSingleton::get_instance()");
        return shared_ptr<ServiceBus>{};  // To avoid warnings
    } else {
        return SERVICE_BUS;
    }
}
