#pragma once

#include <memory>
#include <mutex>

#include "ServiceBus.h"

using namespace std;

namespace service_bus {

/**
 * Wrapper for static initialization and instantiation of ServiceBus.
 *
 * ServiceBus objects are singletons which are supposed to be accessed through this class, which
 * performs proper initialization in addition to manage the control of the singleton instantiation.
 *
 * ServiceBus singleton instance can be obtained by calling get_instance(). However, before calling
 * this method, init() is supposed to be called in order to initialize all the static state.
 */
class ServiceBusSingleton {
   public:
    ~ServiceBusSingleton(){};

    /**
     * Initializes static state of the singleton object.
     *
     * This method is supposed to be called once before any calls to get_instance().
     *
     * @param host_id Network id of the bus element using the bus in the form "host:port".
     * @param known_peer Network id of one of the other elements already in the bus. If this element
     * is supposed to be the first element in the bus, known_peer cam be ommited (or passed "").
     * @param port_lower Lowerbound for port numbers to be used by command proxy (see ServiceBusProxy)
     * @param port_upper Upperbound for port numbers to be used by command proxy (see ServiceBusProxy)
     */
    static void init(const string& host_id,
                     const string& known_peer = "",
                     unsigned int port_lower = 64000,
                     unsigned int port_upper = 64999);

    /**
     * Initializes static state of the singleton object.
     * This method is supposed to be called once before any calls to get_instance().
     * @param service_bus A shared pointer to a ServiceBus instance.
     * This method is used to provide a pre-initialized ServiceBus instance, which can be useful for
     * testing or when the ServiceBus instance is already created and configured.
     */
    static void provide(shared_ptr<ServiceBus> service_bus);

    /**
     * Returns the ServiceBus singleton instance.
     *
     * init() need to be called before calling this method.
     *
     * @return The ServiceBus singleton instance.
     */
    static shared_ptr<ServiceBus> get_instance();

   private:
    ServiceBusSingleton(){};
    static bool INITIALIZED;
    static shared_ptr<ServiceBus> SERVICE_BUS;
    static mutex API_MUTEX;
};

}  // namespace service_bus
