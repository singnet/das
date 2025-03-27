#include "ServiceBusSingleton.h"
#include "Utils.h"

using namespace service_bus;
using namespace commons;

bool ServiceBusSingleton::initialized = false;
shared_ptr<ServiceBus> ServiceBusSingleton::service_bus = shared_ptr<ServiceBus>{};


// -------------------------------------------------------------------------------------------------
// Public methods

void ServiceBusSingleton::init(const string& host_id, const string& known_peer) {
    if (initialized) {
        Utils::error("ServiceBusSingleton already initialized. "
            "ServiceBusSingleton::init() should be called only once.");
    } else {
        ServiceBus::initialize_statics();
        service_bus = shared_ptr<ServiceBus>(new ServiceBus(host_id, known_peer));
        initialized = true;
    }
}

shared_ptr<ServiceBus> ServiceBusSingleton::get_instance() {
    if (! initialized) {
        Utils::error("Uninitialized ServiceBusSingleton. ServiceBusSingleton::init() "
            "must be called before ServiceBusSingleton::get_instance()");
        return shared_ptr<ServiceBus>{}; // To avoid warnings
    } else {
        return service_bus;
    }
}
