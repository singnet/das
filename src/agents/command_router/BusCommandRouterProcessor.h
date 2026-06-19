#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "BaseProxy.h"
#include "BusCommandProcessor.h"
#include "BusCommandRouterProxy.h"
#include "CommandRouterHttpAPI.h"
#include "Properties.h"
#include "QueryAnswer.h"
#include "ServiceBus.h"

using namespace std;
using namespace service_bus;
using namespace agents;
using namespace query_engine;
using namespace processor;

namespace command_router {

class BusCommandRouterProcessor : public BusCommandProcessor {
   public:
    /**
     * @param service_bus Bus used to forward commands. If null, ServiceBusSingleton is used
     * (normal busnode deployment).
     */
    explicit BusCommandRouterProcessor(shared_ptr<ServiceBus> service_bus = nullptr);
    ~BusCommandRouterProcessor() override;

    shared_ptr<BusCommandProxy> factory_empty_proxy() override;
    void run_command(shared_ptr<BusCommandProxy> proxy) override;

   private:
    void handle_get(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);
    void handle_set(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);
    void handle_query(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);
    void handle_evolution(shared_ptr<BusCommandRouterProxy> proxy, const string& arg);

    void forward_to_service(shared_ptr<BusCommandRouterProxy> router_proxy,
                            shared_ptr<BaseQueryProxy> service_proxy);

    void set_router_param(BusCommandRouterProxy* proxy, const string& key, const string& value);

    Properties& parameters_for_peer(const string& peer_id);

    /** Router params keyed by bus requestor_id (client ServiceBus host_id). */
    unordered_map<string, Properties> router_parameters_by_peer;
    mutex router_parameters_mutex;
    shared_ptr<ServiceBus> service_bus;

    shared_ptr<CommandRouterHttpAPI> http_api;
};

}  // namespace command_router
